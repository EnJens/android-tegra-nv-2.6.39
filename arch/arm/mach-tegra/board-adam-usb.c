/*
 * arch/arm/mach-tegra/board-adam-usb.c
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

/* All configurations related to USB */

/* Misc notes on USB bus on Tegra2 systems (extracted from several conversations
held regarding USB devices not being recognized)

  For additional power saving, the tegra ehci USB driver supports powering
down the phy on bus suspend when it is used, for example, to connect an 
internal device that use an out-of-band remote wakeup mechanism (e.g. a 
gpio).

  With power_down_on_bus_suspend = 1, the board fails to recognize newly
attached devices, i.e. only devices connected during boot are accessible.
But it doesn't cause problems with the devices themselves. The main cause
seems to be that power_down_on_bus_suspend = 1 will stop the USB ehci clock
, so we dont get hotplug events.

  Seems that it is needed to keep the IP clocked even if phy is powered 
down on bus suspend, since otherwise we don't get hotplug events for
hub-less systems.

*/
 
#include <linux/kobject.h>
#include <linux/console.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/usb/android_composite.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/i2c-tegra.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>

#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <mach/clk.h>
#include <mach/usb_phy.h>
#include <mach/system.h>

#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/f_accessory.h>

#include "board.h"
#include "board-adam.h"
#include "clock.h"
#include "gpio-names.h"
#include "devices.h"

#ifdef CONFIG_USB_SUPPORT

#define USB_MANUFACTURER_NAME		"NVIDIA"
#define USB_PRODUCT_NAME			"ADAM"
#define USB_PRODUCT_ID_MTP_ADB		0x7100
#define USB_PRODUCT_ID_MTP			0x7102
#define USB_PRODUCT_ID_RNDIS		0x7103
#define USB_PRODUCT_ID_ACM			0x7104
#define USB_VENDOR_ID				0x0955

static char *usb_functions_mtp_ums[] = { 
#ifdef CONFIG_USB_ANDROID_MTP	
	"mtp", 
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	"usb_mass_storage",
#endif
};
	
static char *usb_functions_mtp_adb_ums[] = { 
#ifdef CONFIG_USB_ANDROID_MTP		
	"mtp", 
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb", 
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE	
	"usb_mass_storage",
#endif
};

static char *usb_functions_accessory[] = { 
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
};
	
static char *usb_functions_accessory_adb[] = { 
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb", 
#endif
};

static char *usb_functions_rndis[] = { 
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
};
	
static char *usb_functions_rndis_adb[] = { 
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb", 
#endif
};

static char *usb_functions_acm[] = { 
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
	
static char *usb_functions_acm_adb[] = { 
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb", 
#endif
};

static char *tegra_android_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_MTP
	"mtp",
#endif
#ifdef CONFIG_USB_ANDROID_ACM	
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb",
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	"usb_mass_storage",
#endif
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
};

static struct android_usb_product usb_products[] = {
#if defined(CONFIG_USB_ANDROID_MASS_STORAGE) || defined(CONFIG_USB_ANDROID_MTP)
	{
		.product_id     = USB_PRODUCT_ID_MTP,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp_ums),
		.functions      = usb_functions_mtp_ums,
	},
	{
		.product_id     = USB_PRODUCT_ID_MTP_ADB,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp_adb_ums),
		.functions      = usb_functions_mtp_adb_ums,
	},
#endif
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id      = USB_ACCESSORY_VENDOR_ID,
		.product_id     = USB_ACCESSORY_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_accessory),
		.functions      = usb_functions_accessory,
	},
	{
		.vendor_id      = USB_ACCESSORY_VENDOR_ID,
		.product_id     = USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_accessory_adb),
		.functions      = usb_functions_accessory_adb,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		.product_id     = USB_PRODUCT_ID_RNDIS,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = USB_PRODUCT_ID_RNDIS,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	{
		.product_id     = USB_PRODUCT_ID_ACM,
		.num_functions  = ARRAY_SIZE(usb_functions_acm),
		.functions      = usb_functions_acm,
	},
	{
		.product_id     = USB_PRODUCT_ID_ACM,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_adb),
		.functions      = usb_functions_acm_adb,
	},
#endif
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat = {
	.vendor_id 			= USB_VENDOR_ID,
#if defined(CONFIG_USB_ANDROID_MASS_STORAGE) || defined(CONFIG_USB_ANDROID_MTP)
	.product_id 		= USB_PRODUCT_ID_MTP_ADB,
#elif defined(CONFIG_USB_ANDROID_ACCESSORY)
	.product_id     	= USB_ACCESSORY_PRODUCT_ID,
#elif defined(CONFIG_USB_ANDROID_RNDIS)
	.product_id 		= USB_PRODUCT_ID_RNDIS,
#else
	.product_id 		= USB_PRODUCT_ID_ACM,
#endif
	.manufacturer_name 	= USB_MANUFACTURER_NAME,
	.product_name      	= USB_PRODUCT_NAME,
	.serial_number     	= "0000",
	.num_products 		= ARRAY_SIZE(usb_products),
	.products 			= usb_products,
	.num_functions 		= ARRAY_SIZE(tegra_android_functions_all),
	.functions 			= tegra_android_functions_all,
};

#ifdef CONFIG_USB_ANDROID_ACM	
static struct acm_platform_data tegra_acm_platform_data = {
	.num_inst = 1,
};
static struct platform_device tegra_usb_acm_device = {
	.name 	 = "acm",
	.id 	 = -1,
	.dev = {
		.platform_data = &tegra_acm_platform_data,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
static struct usb_mass_storage_platform_data tegra_ums_platform = {
	.vendor  = "NVIDIA",
	.product = "Tegra 2",
	.nluns 	 = 1,
};
static struct platform_device tegra_usb_ums_device = {
	.name 	 = "usb_mass_storage",
	.id 	 = -1,
	.dev = {
		.platform_data = &tegra_ums_platform,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data tegra_rndis_pdata = {
	.ethaddr 	 = {0x02, 0x04, 0x50, 0x60, 0x08, 0x10},
	.vendorID 	 = USB_VENDOR_ID,
	.vendorDescr = USB_MANUFACTURER_NAME,
};

static struct platform_device tegra_usb_rndis_device = {
	.name   = "rndis",
	.id     = -1,
	.dev    = {
		.platform_data  = &tegra_rndis_pdata,
	},
};
#endif

static struct platform_device androidusb_device = {
	.name   = "android_usb",
	.id     = -1,
	.dev    = {
		.platform_data  = &andusb_plat,
	},
};

static struct tegra_utmip_config utmi_phy_config[] = {
	[0] = {
		.hssync_start_delay = 0,
		.idle_wait_delay 	= 17,
		.elastic_limit 		= 16,
		.term_range_adj 	= 6, 	/*  xcvr_setup = 9 with term_range_adj = 6 gives the maximum guard around */
		.xcvr_setup 		= 9, 	/*  the USB electrical spec. This is true across fast and slow chips, high */
									/*  and low voltage and hot and cold temperatures */
		.xcvr_lsfslew 		= 2,	/*  -> To slow rise and fall times in low speed eye diagrams in host mode */
		.xcvr_lsrslew 		= 2,	/*                                                                        */
	},
	[1] = {
		.hssync_start_delay = 0,
		.idle_wait_delay 	= 17,
		.elastic_limit 		= 16,
		.term_range_adj 	= 6,	/*  -> xcvr_setup = 9 with term_range_adj = 6 gives the maximum guard around */
		.xcvr_setup 		= 9,	/*     the USB electrical spec. This is true across fast and slow chips, high */
									/*     and low voltage and hot and cold temperatures */
		.xcvr_lsfslew 		= 2,	/*  -> To slow rise and fall times in low speed eye diagrams in host mode */
		.xcvr_lsrslew 		= 2,	/*                                                                        */
	},
};

static struct tegra_ehci_platform_data tegra_ehci_pdata[] = {
	[0] = {
		.phy_config = &utmi_phy_config[0],
		.operating_mode = TEGRA_USB_OTG, /* DEVICE is slave here / HOST*/
		.power_down_on_bus_suspend = 0,
	},
	[1] = {
		.phy_config = &utmi_phy_config[1],
		.operating_mode = TEGRA_USB_HOST,
		.power_down_on_bus_suspend = 0,
		.hotplug = 1,
	},
};

struct platform_device *usb_host_pdev = NULL;
static struct platform_device * tegra_usb_otg_host_register(void)
{
	int val;
	struct platform_device *pdev = NULL;
	void *platform_data;

	pr_info("%s: enabling USB host mode\n", __func__);	
	
	/* Enable VBUS - This means we can power USB devices, but
	   we cant use VBUS detection at all */
	gpio_direction_input(ADAM_USB0_VBUS);

	/* Leave some time for stabilization purposes */
	msleep(10);
	
	/* And register the USB host device */
	pdev = platform_device_alloc(tegra_ehci1_device.name,
			tegra_ehci1_device.id);
	if (!pdev)
		goto err_2;

	val = platform_device_add_resources(pdev, tegra_ehci1_device.resource,
		tegra_ehci1_device.num_resources);
	if (val)
		goto error;

	pdev->dev.dma_mask =  tegra_ehci1_device.dev.dma_mask;
	pdev->dev.coherent_dma_mask = tegra_ehci1_device.dev.coherent_dma_mask;
	
	platform_data = kmalloc(sizeof(struct tegra_ehci_platform_data), GFP_KERNEL);
	if (!platform_data)
		goto error;

	memcpy(platform_data, tegra_ehci1_device.dev.platform_data,
				sizeof(struct tegra_ehci_platform_data));
	pdev->dev.platform_data = platform_data;
 	
	val = platform_device_add(pdev);
	if (val)
		goto error_add;

	usb_host_pdev = pdev;
	return pdev;

error_add:
	kfree(platform_data);
error:
	platform_device_put(pdev);
err_2:
	pr_err("%s: failed to add the host controller device\n", __func__);	
	return NULL;
}

static void tegra_usb_otg_host_unregister(struct platform_device *pdev)
{
	pr_info("%s: disabling USB host mode\n", __func__);	

	/* Disable VBUS power. This means that if a USB host
	   is plugged into the Tegra USB port, then we will 
	   detect the power it supplies and go into gadget 
	   mode */
	gpio_direction_output(ADAM_USB0_VBUS, 0); 

	/* Leave some time for stabilization purposes - This 
	   should unregister all attached devices, as they
	   all lost power */
	msleep(500);

	/* Unregister the host adapter */
	kfree(pdev->dev.platform_data);
	pdev->dev.platform_data = NULL; /* This will avoid a crash on device release */
	platform_device_unregister(pdev);
	usb_host_pdev = NULL;
}

#ifdef CONFIG_USB_TEGRA_OTG
static struct tegra_otg_platform_data tegra_otg_pdata = {
	.host_register = &tegra_usb_otg_host_register,
	.host_unregister = &tegra_usb_otg_host_unregister,
};
#endif


static struct platform_device *adam_usb_devices[] __initdata = {
#ifdef CONFIG_USB_TEGRA_OTG
	/* OTG should be the first to be registered */
	&tegra_otg_device,
#endif
#ifdef CONFIG_USB_ANDROID_ACM	
	&tegra_usb_acm_device,
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&tegra_usb_ums_device,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&tegra_usb_rndis_device,
#endif
	&androidusb_device,		/* should come AFTER ums and acm */
	&tegra_udc_device, 		/* USB gadget */
	&tegra_ehci3_device,
};


static void tegra_set_host_mode(void)
{
	/* Place interface in host mode	*/
#ifdef CONFIG_USB_TEGRA_OTG

	/* Switch to host mode */
	tegra_otg_set_host_mode(true);
	
#else

	if (!usb_host_pdev) {
		usb_host_pdev = tegra_usb_otg_host_register();
	}
#endif

}

static void tegra_set_gadget_mode(void)
{
	/* Place interfase in gadget mode */

#ifdef CONFIG_USB_TEGRA_OTG

	/* Switch to peripheral mode */
	tegra_otg_set_host_mode(false);

#else
	if (usb_host_pdev) {
		tegra_usb_otg_host_unregister(usb_host_pdev);
		usb_host_pdev = NULL;
	}
#endif
}

struct kobject *usb_kobj = NULL;

static ssize_t usb_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	int ret = 0;
	
	if (!strcmp(attr->attr.name, "host_mode")) {
		if (usb_host_pdev)
			ret = 1;
	}

	return strlcpy(buf, ret ? "1\n" : "0\n", 3);
}

static ssize_t usb_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long on = simple_strtoul(buf, NULL, 10);

	if (!strcmp(attr->attr.name, "host_mode")) {
		if (on)
			tegra_set_host_mode();
		else
			tegra_set_gadget_mode();
	} 

	return count;
}

static DEVICE_ATTR(host_mode, 0666, usb_read, usb_write); /* Allow everybody to switch mode */

static struct attribute *usb_sysfs_entries[] = {
	&dev_attr_host_mode.attr,
	NULL
};

static struct attribute_group usb_attr_group = {
	.name	= NULL,
	.attrs	= usb_sysfs_entries,
}; 

#endif

int __init adam_usb_register_devices(void)
{
#ifdef CONFIG_USB_SUPPORT
	int ret;
	
	tegra_ehci1_device.dev.platform_data = &tegra_ehci_pdata[0];
	tegra_ehci3_device.dev.platform_data = &tegra_ehci_pdata[1];
#ifdef CONFIG_USB_TEGRA_OTG
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
#endif

	/* If in host mode, set VBUS to 1 */
	gpio_request(ADAM_USB0_VBUS, "USB0 VBUS"); /* VBUS switch, perhaps ? -- Tied to what? -- should require +5v ... */
	
	/* 0 = Gadget */
	gpio_direction_output(ADAM_USB0_VBUS, 0 ); /* Gadget */
	
	ret = platform_add_devices(adam_usb_devices, ARRAY_SIZE(adam_usb_devices));
	if (ret)
		return ret;

	/* Enable gadget mode by default */
	tegra_set_gadget_mode();
		
	/* Register a sysfs interface to let user switch modes */
	usb_kobj = kobject_create_and_add("usbbus", NULL);
	if (!usb_kobj) {
		pr_err("Unable to register USB mode switch");
		return 0;	
	}
	
	
	/* Attach an attribute to the already registered usbbus to let the user switch usb modes */
	return sysfs_create_group(usb_kobj, &usb_attr_group); 
#else
	return 0;
#endif	
}
