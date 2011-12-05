/*
 * arch/arm/mach-tegra/adam-pm-gsm.c
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

/* GSM/UMTS power control via GPIO */
  
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
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

struct adam_pm_gsm_data {
	struct regulator *regulator[2];
	int pre_resume_state;
	int state;
#ifdef CONFIG_PM
	int keep_on_in_suspend;
#endif
	struct rfkill *rfkill;
};

/* Power control */
static void __adam_pm_gsm_toggle_radio(struct device *dev, unsigned int on)
{
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(dev);

	/* Avoid turning it on or off if already in that state */
	if (gsm_data->state == on)
		return;
	
	if (on) {
	
		regulator_enable(gsm_data->regulator[0]);
		regulator_enable(gsm_data->regulator[1]);
	
		gpio_set_value(ADAM_3G_DISABLE, 0);

	} else {
	
//W_DISABLE=PB0
// EN_3G=PJ2
		gpio_set_value(ADAM_3G_DISABLE, 1);
		//adam_3g_gps_poweroff();
				
		regulator_disable(gsm_data->regulator[1]);
		regulator_disable(gsm_data->regulator[0]);
	}
	
	/* store new state */
	gsm_data->state = on;
}

static int gsm_rfkill_set_block(void *data, bool blocked)
{
	struct device *dev = data;
	dev_dbg(dev, "blocked %d\n", blocked);

	__adam_pm_gsm_toggle_radio(dev, !blocked);

	return 0;
}

static const struct rfkill_ops adam_gsm_rfkill_ops = {
       .set_block = gsm_rfkill_set_block,
};

static ssize_t gsm_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	int ret = 0;
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(dev);
	
	if (!strcmp(attr->attr.name, "power_on")) {
		if (gsm_data->state)
			ret = 1;
	} else if (!strcmp(attr->attr.name, "reset")) {
		if (gsm_data->state == 0)
			ret = 1;
	}
#ifdef CONFIG_PM
	else if (!strcmp(attr->attr.name, "keep_on_in_suspend")) {
		ret = gsm_data->keep_on_in_suspend;
	}
#endif 	

	if (!ret) {
		return strlcpy(buf, "0\n", 3);
	} else {
		return strlcpy(buf, "1\n", 3);
	}
}

static ssize_t gsm_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long on = simple_strtoul(buf, NULL, 10);
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(dev);

	if (!strcmp(attr->attr.name, "power_on")) {
		rfkill_set_sw_state(gsm_data->rfkill, on ? 1 : 0);
		__adam_pm_gsm_toggle_radio(dev, on);
	} else if (!strcmp(attr->attr.name, "reset")) {
		/* reset is low-active, so we need to invert */
		__adam_pm_gsm_toggle_radio(dev, !on);
	}
#ifdef CONFIG_PM
	else if (!strcmp(attr->attr.name, "keep_on_in_suspend")) {
		gsm_data->keep_on_in_suspend = on;
	}
#endif 
	return count;
}

static DEVICE_ATTR(power_on, 0644, gsm_read, gsm_write);
static DEVICE_ATTR(reset, 0644, gsm_read, gsm_write);

#ifdef CONFIG_PM
static int adam_gsm_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(&pdev->dev);

	dev_dbg(&pdev->dev, "suspending\n");

	gsm_data->pre_resume_state = gsm_data->state;
	if (!gsm_data->keep_on_in_suspend)
		__adam_pm_gsm_toggle_radio(&pdev->dev, 0);
	else
		dev_warn(&pdev->dev, "keeping GSM ON during suspend\n");
		
	return 0;
}

static int adam_gsm_resume(struct platform_device *pdev)
{
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(&pdev->dev);
	dev_dbg(&pdev->dev, "resuming\n");

	__adam_pm_gsm_toggle_radio(&pdev->dev, gsm_data->pre_resume_state);
	return 0;
}
#else
#define adam_gsm_suspend	NULL
#define adam_gsm_resume		NULL
#endif

static struct attribute *adam_gsm_sysfs_entries[] = {
	&dev_attr_power_on.attr,
	&dev_attr_reset.attr,
	NULL
};

static struct attribute_group adam_gsm_attr_group = {
	.name	= NULL,
	.attrs	= adam_gsm_sysfs_entries,
};

static int __init adam_gsm_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	struct regulator *regulator[2];
	struct adam_pm_gsm_data *gsm_data;
	int ret;

	gsm_data = kzalloc(sizeof(*gsm_data), GFP_KERNEL);
	if (!gsm_data) {
		dev_err(&pdev->dev, "no memory for context\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, gsm_data);

	regulator[0] = regulator_get(&pdev->dev, "avdd_usb_pll");
	if (IS_ERR(regulator[0])) {
		dev_err(&pdev->dev, "unable to get regulator for usb pll\n");
		kfree(gsm_data);
		dev_set_drvdata(&pdev->dev, NULL);
		return -ENODEV;
	}
	gsm_data->regulator[0] = regulator[0];

	regulator[1] = regulator_get(&pdev->dev, "avdd_usb");
	if (IS_ERR(regulator[1])) {
		dev_err(&pdev->dev, "unable to get regulator for usb\n");
		regulator_put(regulator[0]);
		gsm_data->regulator[0] = NULL;
		kfree(gsm_data);
		dev_set_drvdata(&pdev->dev, NULL);
		return -ENODEV;
	}
	gsm_data->regulator[1] = regulator[1];
	
	/* Init control pins */
	gpio_request(ADAM_3G_DISABLE, "gsm_disable");
	gpio_direction_output(ADAM_3G_DISABLE, 1);
//	adam_3g_gps_init();

	/* register rfkill interface */
	rfkill = rfkill_alloc(pdev->name, &pdev->dev, RFKILL_TYPE_WWAN,
                            &adam_gsm_rfkill_ops, &pdev->dev);

	if (!rfkill) {
		dev_err(&pdev->dev, "Failed to allocate rfkill\n");
		regulator_put(regulator[1]);
		gsm_data->regulator[1] = NULL;
		regulator_put(regulator[0]);
		gsm_data->regulator[0] = NULL;
		kfree(gsm_data);
		dev_set_drvdata(&pdev->dev, NULL);
		return -ENOMEM;
	}
	gsm_data->rfkill = rfkill;

	/* Disable bluetooth */
    rfkill_init_sw_state(rfkill, 0);

	ret = rfkill_register(rfkill);
	if (ret) {
		rfkill_destroy(rfkill);
		dev_err(&pdev->dev, "Failed to register rfkill\n");
		return ret;
	}

	dev_info(&pdev->dev, "GSM/UMTS RFKill driver loaded\n");
	
	return sysfs_create_group(&pdev->dev.kobj, &adam_gsm_attr_group);
}

static int adam_gsm_remove(struct platform_device *pdev)
{
	struct adam_pm_gsm_data *gsm_data = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &adam_gsm_attr_group);

	if (!gsm_data)
		return 0;
	
	if (gsm_data->rfkill) {
		rfkill_unregister(gsm_data->rfkill);
		rfkill_destroy(gsm_data->rfkill);
	}

	if (gsm_data->regulator[0] && gsm_data->regulator[1])
		__adam_pm_gsm_toggle_radio(&pdev->dev, 0);

	if (gsm_data->regulator[0]) 
		regulator_put(gsm_data->regulator[0]);
		
	if (gsm_data->regulator[1]) 
		regulator_put(gsm_data->regulator[1]);

	kfree(gsm_data);

	return 0;
}
static struct platform_driver adam_gsm_driver_ops = {
	.probe		= adam_gsm_probe,
	.remove		= adam_gsm_remove,
	.suspend	= adam_gsm_suspend,
	.resume		= adam_gsm_resume,
	.driver		= {
		.name		= "adam-pm-gsm",
	},
};

static int __devinit adam_gsm_init(void)
{
	return platform_driver_register(&adam_gsm_driver_ops);
}

static void adam_gsm_exit(void)
{
	platform_driver_unregister(&adam_gsm_driver_ops);
}

module_init(adam_gsm_init);
module_exit(adam_gsm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo José Tagle <ejtagle@tutopia.com>");
MODULE_DESCRIPTION("Shuttle GSM power management");
