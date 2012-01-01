/*
 * arch/arm/mach-tegra/board-adam-usb.c
 *
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 * Copyright (C) 2011 Jens Andersen <jens.andersen@gmail.com>
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
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/i2c-tegra.h>
#include <linux/mfd/tps6586x.h>

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

#include <linux/usb/android_composite.h>
#include <linux/usb/f_accessory.h>

#include "board.h"
#include "clock.h"
#include "board-adam.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"

static struct usb_mass_storage_platform_data tegra_usb_fsg_platform = {
        .vendor = "NVIDIA",
        .product = "Tegra 2",
        .nluns = 1,
};

static struct platform_device tegra_usb_fsg_device = {
        .name = "usb_mass_storage",
        .id = -1,
        .dev = {
                .platform_data = &tegra_usb_fsg_platform,
        },
};



#define USB_MANUFACTURER_NAME           "NVIDIA"
#define USB_PRODUCT_NAME                "Harmony"
#define USB_PRODUCT_ID_MTP_ADB          0x7100
#define USB_PRODUCT_ID_MTP              0x7102
#define USB_PRODUCT_ID_RNDIS            0x7103
#define USB_VENDOR_ID                   0x0955


static char *usb_functions_mtp_ums[] = { "mtp", "usb_mass_storage" };
static char *usb_functions_mtp_adb_ums[] = { "mtp", "adb", "usb_mass_storage" };
#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = { "accessory" };
static char *usb_functions_accessory_adb[] = { "accessory", "adb" };
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
static char *usb_functions_rndis[] = { "rndis" };
static char *usb_functions_rndis_adb[] = { "rndis", "adb" };
#endif
static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
        "rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACCESSORY
        "accessory",
#endif
        "mtp",
        "adb",
        "usb_mass_storage"
};

static struct android_usb_product usb_products[] = {
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
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat = {
        .vendor_id              = USB_VENDOR_ID,
        .product_id             = USB_PRODUCT_ID_MTP_ADB,
        .manufacturer_name      = USB_MANUFACTURER_NAME,
        .product_name           = USB_PRODUCT_NAME,
        .serial_number          = NULL,
        .num_products = ARRAY_SIZE(usb_products),
        .products = usb_products,
        .num_functions = ARRAY_SIZE(usb_functions_all),
        .functions = usb_functions_all,
};

static struct platform_device androidusb_device = {
	.name   = "android_usb",
	.id     = -1,
	.dev    = {
		.platform_data  = &andusb_plat,
	},
};

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
        .ethaddr = {0, 0, 0, 0, 0, 0},
        .vendorID = USB_VENDOR_ID,
        .vendorDescr = USB_MANUFACTURER_NAME,
};

static struct platform_device rndis_device = {
        .name   = "rndis",
        .id     = -1,
        .dev    = {
                .platform_data  = &rndis_pdata,
        },
};
#endif


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

static struct usb_phy_plat_data tegra_usb_phy_pdata[] = {
        [0] = {
                        .instance = 0,
                        .vbus_irq = TPS6586X_INT_BASE + TPS6586X_INT_USB_DET,
                        .vbus_gpio = TEGRA_GPIO_PB0,
        },
        [1] = {
                        .instance = 1,
                        .vbus_gpio = -1,
        },
        [2] = {
                        .instance = 2,
                        .vbus_gpio = -1,
        },
};


/* ULPI is managed by an SMSC3317 on the Harmony board */
static struct tegra_ulpi_config ulpi_phy_config = {
	// Is this even right?
	.reset_gpio = -1, //ADAM_USB1_RESET,
	.clk = "cdev2",
};

static struct tegra_ehci_platform_data tegra_ehci_pdata[] = {
	[0] = {
		.phy_config = &utmi_phy_config[0],
		.operating_mode = TEGRA_USB_OTG,
		.power_down_on_bus_suspend = 0,
	},
	[1] = {
		.phy_config = &ulpi_phy_config,
		.operating_mode = TEGRA_USB_HOST,
		.power_down_on_bus_suspend = 1,
		.phy_type = TEGRA_USB_PHY_TYPE_LINK_ULPI,
	},
	[2] = {
		.phy_config = &utmi_phy_config[1],
		.operating_mode = TEGRA_USB_HOST,
		.power_down_on_bus_suspend = 1,
		.hotplug = 1,
	},
};


struct platform_device *usb_host_pdev = NULL;

static struct platform_device * tegra_usb_otg_host_register(void)
{
        int val;

        /* If already in host mode, dont try to switch to it again */
        if (usb_host_pdev != NULL)
                return usb_host_pdev;

        /* And register the USB host device */
        usb_host_pdev = platform_device_alloc(tegra_ehci1_device.name,
                        tegra_ehci1_device.id);
        if (!usb_host_pdev)
                goto err_2;

        val = platform_device_add_resources(usb_host_pdev, tegra_ehci1_device.resource,
                tegra_ehci1_device.num_resources);
        if (val)
                goto error;

        usb_host_pdev->dev.dma_mask =  tegra_ehci1_device.dev.dma_mask;
        usb_host_pdev->dev.coherent_dma_mask = tegra_ehci1_device.dev.coherent_dma_mask;
        usb_host_pdev->dev.platform_data = tegra_ehci1_device.dev.platform_data;

        val = platform_device_add(usb_host_pdev);
        if (val)
                goto error_add;

        return usb_host_pdev;

error_add:
error:
        platform_device_put(usb_host_pdev);
        usb_host_pdev = NULL;
err_2:
        pr_err("%s: failed to add the host controller device\n", __func__);
        return NULL;
}

static void tegra_usb_otg_host_unregister(void)
{
        /* Unregister the host adapter */
        if (usb_host_pdev != NULL) {
                platform_device_unregister(usb_host_pdev);
                usb_host_pdev = NULL;
        }

        return;

}

static struct tegra_otg_platform_data tegra_otg_pdata = {
        .host_register = &tegra_usb_otg_host_register,
        .host_unregister = &tegra_usb_otg_host_unregister,
};

#define SERIAL_NUMBER_LENGTH 20
static char usb_serial_num[SERIAL_NUMBER_LENGTH];

int __init adam_usb_register_devices(void)
{
	int ret,i;
	char *src;

        snprintf(usb_serial_num, sizeof(usb_serial_num), "%016llx", tegra_chip_uid());
        andusb_plat.serial_number = kstrdup(usb_serial_num, GFP_KERNEL);

	//tegra_usb_phy_init(tegra_usb_phy_pdata, ARRAY_SIZE(tegra_usb_phy_pdata));
        /* OTG should be the first to be registered */
        tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
        platform_device_register(&tegra_otg_device);

        platform_device_register(&tegra_usb_fsg_device);
        platform_device_register(&androidusb_device);
        platform_device_register(&tegra_udc_device);
//        platform_device_register(&tegra_ehci2_device);

        tegra_ehci3_device.dev.platform_data=&tegra_ehci_pdata[2];
        platform_device_register(&tegra_ehci3_device);
#ifdef CONFIG_USB_ANDROID_RNDIS
        src = usb_serial_num;

        /* create a fake MAC address from our serial number.
         * first byte is 0x02 to signify locally administered.
         */
        rndis_pdata.ethaddr[0] = 0x02;
        for (i = 0; *src; i++) {
                /* XOR the USB serial across the remaining bytes */
                rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
        }
        platform_device_register(&rndis_device);
#endif
	tegra_otg_set_host_mode(false);
}
