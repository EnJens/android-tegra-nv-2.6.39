/*
 * arch/arm/mach-tegra/board-ventana-misc.c
 *
 * Copyright (C) 2010-2011 ASUSTek Computer Incorporation
 * Author: Paris Yeh <paris_yeh@asus.com>
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <mach/board-ventana-misc.h>
#include <mach/tegra2_fuse.h>
#include "gpio-names.h"

#define VENTANA_MISC_ATTR(module) \
static struct kobj_attribute module##_attr = { \
	.attr = { \
		.name = __stringify(module), \
		.mode = 0444, \
	}, \
	.show = module##_show, \
}
unsigned int ventana_hw;
/* This flag is set when there is an invalid ext4 partition,
 * init daemon will format data partition when it cannot mount data
 * and this flag is also true.*/
extern int ventana_ext4_invalid;
//Chip unique ID is a maximum of 17 characters including NULL termination.
unsigned char ventana_chipid[17];
EXPORT_SYMBOL(ventana_chipid);

static int __init ventanamisc_setup(char *options)
{
	char *p = options;
	unsigned long ret;

	if (!options)
		return 0;

	ret = simple_strtoul(p, NULL, 0);
	ventana_hw = (unsigned int) ret;
	pr_debug("MISC: ventana_hw=%02x\n", ventana_hw);

	return 1;
}

__setup("hw=", ventanamisc_setup);

unsigned int ASUSGetProjectID()
{
	unsigned int ret = 0;
	ret = HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, PROJECT, ventana_hw);
	switch (ret) {
	case 0: //TF101(EP101)
	case 1: //SL101(EP102)
		ret += 101;
		break;
	default:
		pr_err("[MISC]: Illegal project identification.\n");
	}

	return ret;
}

EXPORT_SYMBOL(ASUSGetProjectID);

unsigned int ASUS3GAvailable()
{
	unsigned int ret = 0;

	if (HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, PROJECT, ventana_hw) < 2) {
		//All valid projects (TF101/SL101/JN101) have 3G SKU definition
		return HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, SKU, ventana_hw);
	}

	return ret;
}

EXPORT_SYMBOL(ASUS3GAvailable);

unsigned int ASUSCheckWLANVendor(unsigned int vendor)
{
	unsigned int ret = 0;

	if (HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, PROJECT, ventana_hw) < 2) {
		//All valid projects (TF101/SL101) have BT/WLAN module
		//definition
		switch (vendor) {
		case BT_WLAN_VENDOR_MURATA:
			ret = HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, VENDOR,
				ventana_hw) ? 0 : 1;
			break;
		case BT_WLAN_VENDOR_AZW:
			ret = HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, VENDOR,
				ventana_hw) ? 1 : 0;
			break;
		default:
			pr_err("[MISC]: Check WLAN with undefined vendor.\n");
		}
        }

	return ret;
}
EXPORT_SYMBOL(ASUSCheckWLANVendor);

unsigned int ASUSCheckTouchVendor(unsigned int vendor)
{
	unsigned int ret = 0;

	if (HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, PROJECT, ventana_hw) < 2) {
		//All valid projects (TF101/SL101) have touch panel module
		//definition
		switch (vendor) {
		case TOUCH_VENDOR_SINTEK:
			ret = HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, TOUCH,
				ventana_hw) ? 0 : 1;
			break;
		case TOUCH_VENDOR_WINTEK:
			ret = HW_DRF_VAL(TEGRA_DEVKIT, MISC_HW, TOUCH,
				ventana_hw) ? 1 : 0;
			break;
		default:
			pr_err("[MISC]: Check TOUCH with undefined vendor.\n");
		}
        }

	return ret;
}
EXPORT_SYMBOL(ASUSCheckTouchVendor);

static ssize_t ventana_ext4_invalid_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
        char *s = buf;

        s += sprintf(s, "%01u\n", ventana_ext4_invalid);

        return (s - buf);
}

static ssize_t ventana_hw_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;

	s += sprintf(s, "%02x\n", ventana_hw);

	return (s - buf);
}

static ssize_t ventana_chipid_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n", ventana_chipid);
	return (s - buf);
}

static ssize_t ventana_projectid_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;

	s += sprintf(s, "%u\n", ASUSGetProjectID());
	return (s - buf);
}

extern void serial8250_console_unregister();
extern struct platform_device tegra_uartd_device;
extern struct platform_device debug_uart;
extern int console_none_on_cmdline;
static int first_attempt = 1;
extern unsigned int g_NvBootArgsGPSPreemption;

/*
 * Export a interface to user space for GPS function/debug console switch.
 *
 * The procedure will check if the target platform is JN101(EP103) and
 * "console=none" is no specified in kernel cmdline.
 * After that, check the input param and GPS preemption ATAG passed by boot
 * loader. If input param is "OFF" and GPS preemption ATAG is specified,
 * release UARTD interface and register with GPS for further usage forever.
 */
static ssize_t ventana_debug_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = count;

	if (ASUSGetProjectID() != 103) {
		pr_info("[MISC]: Refuse any params due to mismatch project\n");
		return ret;
	}

	// return immediately if no console is specified in kernel command line
	if (console_none_on_cmdline) {
		pr_info("[MISC]: Debug console is already disabled\n");
		return ret;
	}

	if (!strncmp(buf, "ON", strlen("ON"))) {
		//put a place holder here, no implementation yet
		//pr_info("[MISC]: Enable debug console\n");
		//platform_device_del(&tegra_uartd_device);
		//gpio_direction_output(TEGRA_GPIO_PS6, 1);
		//platform_device_register(&debug_uart);
	}
	else if ((!strncmp(buf, "OFF", strlen("OFF"))) && first_attempt) {
		first_attempt = 0;
		if (!g_NvBootArgsGPSPreemption) {
			pr_info("[MISC]: Forbid GPS preemption\n");
			return ret;
		}
		pr_info("[MISC]: Disable debug console\n");
		serial8250_console_unregister();
		platform_device_del(&debug_uart);
		gpio_direction_output(TEGRA_GPIO_PS6, 0);
		platform_device_register(&tegra_uartd_device);
	}
	else
		pr_err("[MISC]: Undefined commands!\n");

	return ret;
}

static ssize_t ventana_fuse_reservedodm_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	unsigned int odm_rsvd[8] = {0};
	unsigned char *value = (unsigned char *)odm_rsvd;
	int count = 0;

	if (!tegra_fuse_read(ODM_RSVD, odm_rsvd, sizeof(odm_rsvd))) {
		while (count < sizeof(odm_rsvd)) {
			s += sprintf(s, "%02x", value[count]);
			count++;
		}

		s += sprintf(s, "\n");
	}
	else
		pr_err("[MISC]: Cannot query Tegra2 ReservedOdm fuse\n");

	return (s - buf);
}

VENTANA_MISC_ATTR(ventana_hw);
VENTANA_MISC_ATTR(ventana_chipid);
VENTANA_MISC_ATTR(ventana_projectid);
VENTANA_MISC_ATTR(ventana_fuse_reservedodm);
VENTANA_MISC_ATTR(ventana_ext4_invalid);

static struct kobj_attribute ventana_debug_attr = {
	.attr = {
		.name = __stringify(ventana_debug),
		.mode = 0220,
	},
	.store = ventana_debug_store,
};

static struct attribute *attr_list[] = {
	&ventana_hw_attr.attr,
	&ventana_chipid_attr.attr,
	&ventana_projectid_attr.attr,
	&ventana_fuse_reservedodm_attr.attr,
	&ventana_debug_attr.attr,
        &ventana_ext4_invalid_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attr_list,
};

static struct platform_device *ventana_misc_device;

int __init ventana_setup_misc(void)
{
	int ret = 0;

	pr_debug("%s: start\n", __func__);

	ventana_misc_device = platform_device_alloc("ventana_misc", -1);

        if (!ventana_misc_device) {
		ret = -ENOMEM;
		goto fail_platform_device;
        }

	ret = platform_device_add(ventana_misc_device);
	if (ret) {
		pr_err("[MISC]: cannot add device to platform.\n");
		goto fail_platform_add_device;
	}

	ret = sysfs_create_group(&ventana_misc_device->dev.kobj, &attr_group);
	if (ret) {
		pr_err("[MISC]: cannot create sysfs group.\n");
		goto fail_sysfs;
	}

	return ret;

fail_sysfs:
	platform_device_del(ventana_misc_device);

fail_platform_add_device:
	platform_device_put(ventana_misc_device);

fail_platform_device:
	return ret;
}
