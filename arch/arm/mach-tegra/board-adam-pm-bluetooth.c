/*
 * Bluetooth Broadcomm  and low power control via GPIO
 *
 *
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */ 

/* Bluetooth is an HCI UART device attached to UART2, and requires a 32khz blink clock */
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <linux/rfkill.h>

#include "board-adam.h"
#include "gpio-names.h"

#include <mach/hardware.h>
#include <asm/mach-types.h>

struct adam_pm_bt_data {
	struct regulator *regulator;
	struct clk *clk;
	struct rfkill *rfkill;
	int pre_resume_state;
	int state;
};

/* Power control */
static void __adam_pm_bt_toggle_radio(struct device *dev, unsigned int on)
{
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(dev);

	/* Avoid turning it on if already on */
	if (bt_data->state == on)
		return;
	
	if (on) {
		dev_info(dev, "Enabling Bluetooth\n");
	
		regulator_enable(bt_data->regulator);
		clk_enable(bt_data->clk);
	
		/* Bluetooth power on sequence */
		gpio_set_value(ADAM_BT_RESET, 0); /* Assert reset */
		msleep(200);
		gpio_set_value(ADAM_BT_RESET, 1); /* Deassert reset */
		msleep(2);

	} else {
		dev_info(dev, "Disabling Bluetooth\n");
		
		gpio_set_value(ADAM_BT_RESET, 0); /* Assert reset */
		
		clk_disable(bt_data->clk);
		regulator_disable(bt_data->regulator);
	}
	
	/* store new state */
	bt_data->state = on;
}

static int bt_rfkill_set_block(void *data, bool blocked)
{
	struct device *dev = data;
	dev_dbg(dev, "blocked %d\n", blocked);

	__adam_pm_bt_toggle_radio(dev, !blocked);

	return 0;
}

static const struct rfkill_ops adam_bt_rfkill_ops = {
       .set_block = bt_rfkill_set_block,
};

static ssize_t bt_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	int ret = 0;
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(dev);
	
	if (!strcmp(attr->attr.name, "power_on")) {
		if (bt_data->state)
			ret = 1;
	} else if (!strcmp(attr->attr.name, "reset")) {
		if (bt_data->state == 0)
			ret = 1;
	}

	if (!ret) {
		return strlcpy(buf, "0\n", 3);
	} else {
		return strlcpy(buf, "1\n", 3);
	}
}

static ssize_t bt_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long on = simple_strtoul(buf, NULL, 10);
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(dev);

	if (!strcmp(attr->attr.name, "power_on")) {
		rfkill_set_sw_state(bt_data->rfkill, on ? 1 : 0);
		__adam_pm_bt_toggle_radio(dev, on);
	} else if (!strcmp(attr->attr.name, "reset")) {
		/* reset is low-active, so we need to invert */
		__adam_pm_bt_toggle_radio(dev, !on);
	}

	return count;
}

static DEVICE_ATTR(power_on, 0644, bt_read, bt_write);
static DEVICE_ATTR(reset, 0644, bt_read, bt_write);

#ifdef CONFIG_PM
static int adam_bt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(&pdev->dev);

	dev_dbg(&pdev->dev, "suspending\n");

	bt_data->pre_resume_state = bt_data->state;
	__adam_pm_bt_toggle_radio(&pdev->dev, 0);

	return 0;
}

static int adam_bt_resume(struct platform_device *pdev)
{
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(&pdev->dev);
	dev_dbg(&pdev->dev, "resuming\n");

	__adam_pm_bt_toggle_radio(&pdev->dev, bt_data->pre_resume_state);
	return 0;
}
#else
#define adam_bt_suspend	NULL
#define adam_bt_resume	NULL
#endif

static struct attribute *adam_bt_sysfs_entries[] = {
	&dev_attr_power_on.attr,
	&dev_attr_reset.attr,
	NULL
};

static struct attribute_group adam_bt_attr_group = {
	.name	= NULL,
	.attrs	= adam_bt_sysfs_entries,
};

static int __init adam_bt_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	struct regulator *regulator;
	struct clk *clk;
	struct adam_pm_bt_data *bt_data;
	int ret;

	dev_dbg(&pdev->dev, "starting\n");

	bt_data = kzalloc(sizeof(*bt_data), GFP_KERNEL);
	if (!bt_data) {
		dev_err(&pdev->dev, "no memory for context\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, bt_data);

	regulator = regulator_get(&pdev->dev, "vddhostif_bt");
	if (IS_ERR(regulator)) {
		dev_err(&pdev->dev, "Failed to get regulator\n");
		ret = -ENODEV;
		goto err4;
	}
	bt_data->regulator = regulator;
	
	clk = clk_get(&pdev->dev, "blink");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Failed to get clock\n");
		ret = -ENODEV;
		goto err3;
	}
	bt_data->clk = clk;

	/* Init io pins */
	gpio_request(ADAM_BT_RESET, "bluetooth_reset");
	gpio_direction_output(ADAM_BT_RESET, 0);
	
	rfkill = rfkill_alloc(pdev->name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
                            &adam_bt_rfkill_ops, &pdev->dev);

	if (!rfkill) {
		dev_err(&pdev->dev, "Failed to allocate rfkill\n");
		ret = -ENOMEM;
		goto err2;
	}
	bt_data->rfkill = rfkill;

	/* Disable bluetooth */
    rfkill_init_sw_state(rfkill, 0);

	ret = rfkill_register(rfkill);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register rfkill\n");
		goto err1;
	}

	dev_info(&pdev->dev, "Bluetooth RFKill driver registered\n");	
	
	return sysfs_create_group(&pdev->dev.kobj, &adam_bt_attr_group);
	
err1:
	rfkill_destroy(rfkill);
	bt_data->rfkill = NULL;
err2:
	clk_put(clk);
	bt_data->clk = NULL;
err3:
	regulator_put(regulator);
	bt_data->regulator = NULL;
err4:
	kfree(bt_data);
	dev_set_drvdata(&pdev->dev, NULL);
	return ret;
}

static int adam_bt_remove(struct platform_device *pdev)
{
	struct adam_pm_bt_data *bt_data = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &adam_bt_attr_group);

	if (bt_data->rfkill) {
		rfkill_unregister(bt_data->rfkill);
		rfkill_destroy(bt_data->rfkill);
	}

	if (!bt_data || !bt_data->regulator)
		return 0;

	__adam_pm_bt_toggle_radio(&pdev->dev, 0);

	regulator_put(bt_data->regulator);
	clk_put(bt_data->clk);

	kfree(bt_data);
	return 0;
}
static struct platform_driver adam_bt_driver_ops = {
	.probe		= adam_bt_probe,
	.remove		= adam_bt_remove,
	.suspend	= adam_bt_suspend,
	.resume		= adam_bt_resume,
	.driver		= {
		.name		= "adam-pm-bt",
	},
};

static int __devinit adam_bt_init(void)
{
	return platform_driver_register(&adam_bt_driver_ops);
}

static void adam_bt_exit(void)
{
	platform_driver_unregister(&adam_bt_driver_ops);
}

module_init(adam_bt_init);
module_exit(adam_bt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo José Tagle <ejtagle@tutopia.com>");
MODULE_DESCRIPTION("Shuttle Bluetooth power management");


 
