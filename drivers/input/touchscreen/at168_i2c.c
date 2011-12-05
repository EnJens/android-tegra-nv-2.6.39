/*
 * drivers/input/touchscreen/at168_i2c.c
 *
 * Touchscreen class input driver for AT168 touch panel using I2C bus
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 * Copyright (c) 2011, Jens Andersen <jens.andersen@gmail.com>
 * This driver is based on Panjit I2C driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/i2c/at168_ts.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>


#define DRIVER_NAME	"at168_touch"
#define TS_POLL_DELAY			1 /* ms delay between samples */
#define TS_POLL_PERIOD			1 /* ms delay between samples */

#ifdef CONFIG_HAS_EARLYSUSPEND
static void at168_early_suspend(struct early_suspend *h);
static void at168_late_resume(struct early_suspend *h);
#endif


struct at168_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	struct i2c_client 	*read_client;
	int			gpio_reset;
	int 		gpio_irq;
	struct early_suspend	early_suspend;
	struct delayed_work	work;
};

struct at168_event {
	__u8	fingers;
	__u8	old_fingers;
	__be16	coord[2][2];
};

union at168_buff {
	struct at168_event	data;
	unsigned char	buff[sizeof(struct at168_event)];
};

static int at168_read_registers(struct at168_data *touch, unsigned char reg, unsigned char* buffer, unsigned int len);

static void at168_reset(struct at168_data *touch)
{
	if (touch->gpio_reset < 0)
		return;

	gpio_set_value(touch->gpio_reset, 0);
	msleep(5);
	gpio_set_value(touch->gpio_reset, 1);
	msleep(60);
}

static void at168_work(struct work_struct *work)
{
	struct at168_data *touch =
		container_of(to_delayed_work(work), struct at168_data, work);
	union at168_buff event;
	int ret,i;
	
	ret = at168_read_registers(touch, AT168_TOUCH_NUM, event.buff, sizeof(event));
	
	input_report_key(touch->input_dev, BTN_TOUCH, (event.data.fingers == 1 || event.data.fingers == 2) );
	input_report_key(touch->input_dev, BTN_2, event.data.fingers == 2);

	/*if (!event.data.fingers || (event.data.fingers > 2))
		goto out;*/

	for (i = 0; i < event.data.fingers; i++) {
		input_report_abs(touch->input_dev, ABS_MT_POSITION_X,
				 event.data.coord[i][0]);
		input_report_abs(touch->input_dev, ABS_MT_POSITION_Y,
				 event.data.coord[i][1]);
		//input_report_abs(touch->input_dev, ABS_MT_TRACKING_ID, i);
		input_report_abs(touch->input_dev, ABS_MT_TOUCH_MAJOR, 10);
		input_report_abs(touch->input_dev, ABS_MT_WIDTH_MAJOR, 20);
		input_mt_sync(touch->input_dev);
	}
	if(event.data.fingers == 0)
	{
		input_report_abs(touch->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(touch->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_mt_sync(touch->input_dev);
	}
out:
	input_sync(touch->input_dev);
	if (event.data.fingers > 0)
		schedule_delayed_work(&touch->work,
				      msecs_to_jiffies(TS_POLL_PERIOD));
	else
		enable_irq(touch->gpio_irq);
}

static irqreturn_t at168_irq(int irq, void *dev_id)
{
	struct at168_data *ts = dev_id;
	disable_irq_nosync(ts->gpio_irq);
	schedule_delayed_work(&ts->work,
		      msecs_to_jiffies(TS_POLL_DELAY));

	return IRQ_HANDLED;
}

static int at168_read_registers(struct at168_data *touch, unsigned char reg, unsigned char* buffer, unsigned int len)
{
	int ret;
	struct i2c_msg msgs[2];
	msgs[0].addr = touch->client->addr;
	msgs[0].len = 1;
	msgs[0].buf = &reg;
	msgs[0].flags = 0;
	
	msgs[1].addr = touch->client->addr;
	msgs[1].len=len;
	msgs[1].buf = buffer;
	msgs[1].flags = I2C_M_RD;
	
	do
	{
		ret = i2c_transfer(touch->client->adapter, msgs, 2);
	} while(ret == EAGAIN || ret == ETIMEDOUT);
	return ret;
}

static ssize_t at168_dump_registers(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret,i,written=0;
	#define NUM_REGISTERS (AT168_VERSION_PROTOCOL - AT168_TOUCH_NUM + 1)
	unsigned char initdata[NUM_REGISTERS] = {};
	char *curr_buf;
	struct at168_data *touch = i2c_get_clientdata(to_i2c_client(dev));
	ret = at168_read_registers(touch, AT168_TOUCH_NUM, initdata, NUM_REGISTERS);
	int cnt=0;
	if(ret > 0)
	{
		curr_buf = buf;
		for(i=0; i<NUM_REGISTERS; i++)
		{
			written = sprintf(curr_buf, "R[0x%02X] = %d\n", i+1, initdata[i]);
			cnt += written;
			curr_buf += written;
		}
	}
	return written;
}


static DEVICE_ATTR(dump_registers, 0664, at168_dump_registers, NULL);

static int at168_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct at168_i2c_ts_platform_data *pdata = client->dev.platform_data;
	struct at168_data *touch = NULL;
	struct input_dev *input_dev = NULL;
	unsigned char initdata[AT168_VERSION_PROTOCOL - AT168_XMAX_LO + 1] = {};
	int ret = 0;
	int j = 0;

	touch = kzalloc(sizeof(struct at168_data), GFP_KERNEL);
	if (!touch) {
		dev_err(&client->dev, "%s: no memory\n", __func__);
		return -ENOMEM;
	}
	gpio_request(pdata->gpio_power, "at168_ts_power");
	gpio_direction_output(pdata->gpio_power, 1);

	touch->gpio_reset = -EINVAL;

	if (pdata) {
		ret = gpio_request(pdata->gpio_reset, "at168_ts_reset");
		if (!ret) {
			ret = gpio_direction_output(pdata->gpio_reset, 0);
			if (ret < 0)
				gpio_free(pdata->gpio_reset);
		}

		if (!ret)
			touch->gpio_reset = pdata->gpio_reset;
		else
			dev_warn(&client->dev, "unable to configure GPIO\n");
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "%s: no memory\n", __func__);
		kfree(touch);
		return -ENOMEM;
	}

	touch->client = client;
	i2c_set_clientdata(client, touch);
	INIT_DELAYED_WORK(&touch->work, at168_work);
	
	at168_reset(touch);

	
	ret = at168_read_registers(touch, AT168_XMAX_LO, initdata, (AT168_VERSION_PROTOCOL - AT168_XMAX_LO + 1));

	
	do
	{
		printk("at168_touch: InitData[%d] = 0x%x---\n", j, initdata[j]);
		j++;
	}while(j < 8);
	
	int version = ((initdata[4] << 24) | (initdata[5] << 16) | (initdata[6] << 8) | (initdata[7]) );
	/*int XMinPosition = 0; //AT168_MIN_X;
	int YMinPosition = 0; //AT168_MIN_Y;*/
	int XMaxPosition = ((initdata[1] << 8) | (initdata[0])); //AT168_MAX_X;
	int YMaxPosition = ((initdata[3] << 8) | (initdata[2])); //AT168_MAX_Y;

	printk("at168_touch: now FW xMAX is %d   yMAx is %d Version is %x.\n",XMaxPosition, YMaxPosition, version);

	touch->input_dev = input_dev;
	touch->input_dev->name = DRIVER_NAME;

	set_bit(EV_SYN, touch->input_dev->evbit);
	set_bit(EV_KEY, touch->input_dev->evbit);
	set_bit(EV_ABS, touch->input_dev->evbit);
	set_bit(BTN_TOUCH, touch->input_dev->keybit);
	set_bit(BTN_2, touch->input_dev->keybit);

	/* expose multi-touch capabilities */
	set_bit(ABS_MT_POSITION_X, touch->input_dev->keybit);
	set_bit(ABS_MT_POSITION_Y, touch->input_dev->keybit);
	set_bit(ABS_X, touch->input_dev->keybit);
	set_bit(ABS_Y, touch->input_dev->keybit);

	input_set_abs_params(touch->input_dev, ABS_X, 0, XMaxPosition, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_Y, 0, YMaxPosition, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_HAT0X, 0, XMaxPosition, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_HAT0Y, 0, YMaxPosition, 0, 0);
	/*input_set_abs_params(touch->input_dev, ABS_HAT1X, 0, XMaxPosition, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_HAT1Y, 0, YMaxPosition, 0, 0);*/

	input_set_abs_params(touch->input_dev, ABS_MT_POSITION_X, 0, XMaxPosition, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_MT_POSITION_Y, 0, YMaxPosition, 0, 0); 
	input_set_abs_params(touch->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(touch->input_dev, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
	//input_set_abs_params(touch->input_dev, ABS_MT_TRACKING_ID, 0, 1, 1, 0);
	

	ret = input_register_device(touch->input_dev);
	if (ret) {
		dev_err(&client->dev, "%s: input_register_device failed\n",
			__func__);
		goto fail_i2c_or_register;
	}

	
	/* get the irq */
	ret = request_threaded_irq(touch->client->irq, NULL, at168_irq,
			IRQF_ONESHOT | IRQF_TRIGGER_LOW,
			DRIVER_NAME, touch);
	if(ret) 
	{
			dev_err(&client->dev, "%s: request_irq(%d) failed\n",
				__func__, touch->client->irq);
			goto fail_irq;
	} else
		touch->gpio_irq = touch->client->irq;

	/* todo: rewrite to proper debug output device. See pinmux.c */
/*	ret = device_create_file(&touch->input_dev->dev, &dev_attr_dump_registers);
        if (ret) {
                ret = -ENOMEM;
                dev_err(&client->dev, "error creating calibration attribute\n");
                goto fail_irq;
        }*/


#ifdef CONFIG_HAS_EARLYSUSPEND
	touch->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	touch->early_suspend.suspend = at168_early_suspend;
	touch->early_suspend.resume = at168_late_resume;
	register_early_suspend(&touch->early_suspend);
#endif
	dev_info(&client->dev, "%s: initialized\n", __func__);
	return 0;

fail_irq:
	input_unregister_device(touch->input_dev);
fail_i2c_or_register:
	if (touch->gpio_reset >= 0)
		gpio_free(touch->gpio_reset);

	input_free_device(input_dev);
	kfree(touch);
	return ret;
}

static int at168_suspend(struct i2c_client *client, pm_message_t state)
{
	struct at168_data *touch = i2c_get_clientdata(client);
	int ret;

	if (WARN_ON(!touch))
		return -EINVAL;

	disable_irq(client->irq);

	/* disable scanning and enable deep sleep */
/*	ret = i2c_smbus_write_byte_data(client, CSR, CSR_SLEEP_EN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: sleep enable fail\n", __func__);
		return ret;
	}*/

	return 0;
}

static int at168_resume(struct i2c_client *client)
{
	struct at168_data *touch = i2c_get_clientdata(client);
	int ret = 0;

	if (WARN_ON(!touch))
		return -EINVAL;

	at168_reset(touch);

	/* enable scanning and disable deep sleep */
/*	ret = i2c_smbus_write_byte_data(client, C_FLAG, 0);
	if (ret >= 0)
		ret = i2c_smbus_write_byte_data(client, CSR, CSR_SCAN_EN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: scan enable fail\n", __func__);
		return ret;
	}*/

	enable_irq(client->irq);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void at168_early_suspend(struct early_suspend *es)
{
	struct at168_data *touch;
	touch = container_of(es, struct at168_data, early_suspend);

	if (at168_suspend(touch->client, PMSG_SUSPEND) != 0)
		dev_err(&touch->client->dev, "%s: failed\n", __func__);
}

static void at168_late_resume(struct early_suspend *es)
{
	struct at168_data *touch;
	touch = container_of(es, struct at168_data, early_suspend);

	if (at168_resume(touch->client) != 0)
		dev_err(&touch->client->dev, "%s: failed\n", __func__);
}
#endif

static int at168_remove(struct i2c_client *client)
{
	struct at168_data *touch = i2c_get_clientdata(client);

	if (!touch)
		return -EINVAL;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&touch->early_suspend);
#endif
	free_irq(touch->client->irq, touch);
	if (touch->gpio_reset >= 0)
		gpio_free(touch->gpio_reset);
	i2c_unregister_device(touch->read_client);
	input_unregister_device(touch->input_dev);
	input_free_device(touch->input_dev);
	kfree(touch);
	return 0;
}

static const struct i2c_device_id at168_ts_id[] = {
	{ DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver at168_driver = {
	.probe		= at168_probe,
	.remove		= at168_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= at168_suspend,
	.resume		= at168_resume,
#endif
	.id_table	= at168_ts_id,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __devinit at168_init(void)
{
	int e;

	e = i2c_add_driver(&at168_driver);
	if (e != 0) {
		pr_err("%s: failed to register with I2C bus with "
		       "error: 0x%x\n", __func__, e);
	}
	return e;
}

static void __exit at168_exit(void)
{
	i2c_del_driver(&at168_driver);
}

module_init(at168_init);
module_exit(at168_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AT168 I2C touch driver");
