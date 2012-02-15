/*
 * arch/arm/mach-tegra/board-adam-sdhci.c
 *
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com> 
 * Copyright (C) 2010 Google, Inc.
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
#define DEBUG 1
#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/wlan_plat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/version.h>

#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>
#include <mach/pinmux.h>

#include "gpio-names.h"
#include "devices.h"
#include "board-adam.h"


static void (*wlan_status_cb)(int card_present, void *dev_id) = NULL;
static void *wlan_status_cb_devid = NULL;
static int adam_wlan_cd = 0; /* WIFI virtual 'card detect' status */

static int adam_wifi_status_register(void (*callback)(int , void *), void *);
static struct clk *wifi_32k_clk;

static int adam_wifi_reset(int on);
static int adam_wifi_power(int on);
static int adam_wifi_set_carddetect(int val);

static struct wifi_platform_data adam_wifi_control = {
        .set_power      = adam_wifi_power,
        .set_reset      = adam_wifi_reset,
        .set_carddetect = adam_wifi_set_carddetect,
};


static struct platform_device adam_wifi_device = {
        .name           = "bcmdhd_wlan",
        .id             = 1,
        .dev            = {
                .platform_data = &adam_wifi_control,
        },
};


/* 2.6.36 version has a hook to check card status. Use it */
/*static unsigned int adam_wlan_status(struct device *dev)
{
	return adam_wlan_cd;
}*/

static int adam_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wlan_status_cb)
		return -EAGAIN;
	wlan_status_cb = callback;
	wlan_status_cb_devid = dev_id;
	return 0;
} 
static struct embedded_sdio_data embedded_sdio_data0 = {
        .cccr   = {
                .sdio_vsn       = 2,
                .multi_block    = 1,
                .low_speed      = 0,
                .wide_bus       = 0,
                .high_power     = 1,
                .high_speed     = 1,
        },
        .cis  = {
                .vendor         = 0x02d0,
                .device         = 0x4329,
        },
};

struct tegra_sdhci_platform_data adam_wlan_data = {
//        .clk_id = NULL,
//        .force_hs = 0,
	.mmc_data = {
        	.register_status_notify = adam_wifi_status_register,
//		.embedded_sdio = &embedded_sdio_data0,
//		.built_in = 1,
	},
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.has_no_vreg = 1,
};

/* Used to set the virtual CD of wifi adapter */
int adam_wifi_set_carddetect(int val)
{
	/* Only if a change is detected */
	if (adam_wlan_cd != val) {
	
		/* Store new card 'detect' */
		adam_wlan_cd = val;
		
		/* Let the SDIO infrastructure know about the change */
		if (wlan_status_cb) {
			wlan_status_cb(val, wlan_status_cb_devid);
		} else
			pr_info("%s: Nobody to notify\n", __func__);
	}
	return 0;
}

static int adam_wifi_power(int on)
{
        pr_debug("%s: %d\n", __func__, on);

	adam_bt_wifi_gpio_set(on);
        gpio_set_value(ADAM_WLAN_RESET, on);
        mdelay(200);

        return 0;
}

static int adam_wifi_reset(int on)
{
	gpio_set_value(ADAM_WLAN_RESET, !on);
        pr_debug("%s: %d\n", __func__, on);
//	pr_debug("%s: do nothing, on = %d\n", __func__, on);
        return 0;
}


static struct tegra_sdhci_platform_data tegra_sdhci_platform_data2 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.has_no_vreg = 1,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data3 = {
	.cd_gpio = ADAM_SDHC_CD,
	.wp_gpio = -1,
	.power_gpio = ADAM_SDHC_POWER,
	.has_no_vreg = 1,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data4 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.has_no_vreg = 1,
};



static struct platform_device *adam_sdhci_devices[] __initdata = {
	&tegra_sdhci_device1,
//	&tegra_sdhci_device2,
//have to init these out of order so that the eMMC card is registered first
	&tegra_sdhci_device4,
	&tegra_sdhci_device3,
};

static int __init adam_wifi_init(void)
{
	// Init the power GPIO if it isn't already
	adam_bt_wifi_gpio_init();
        tegra_gpio_enable(ADAM_WLAN_RESET);

	gpio_request(ADAM_WLAN_RESET, "wifi_reset");
        gpio_direction_output(ADAM_WLAN_RESET, 0);

        platform_device_register(&adam_wifi_device);

        device_init_wakeup(&adam_wifi_device.dev, 1);
        device_set_wakeup_enable(&adam_wifi_device.dev, 0);

        return 0;
}


/* Register sdhci devices */
int __init adam_sdhci_register_devices(void)
{
	int ret=0;
	/* Plug in platform data */
	tegra_sdhci_device1.dev.platform_data = &adam_wlan_data;
	tegra_sdhci_device2.dev.platform_data = &tegra_sdhci_platform_data2;
	tegra_sdhci_device3.dev.platform_data = &tegra_sdhci_platform_data3;
	tegra_sdhci_device4.dev.platform_data = &tegra_sdhci_platform_data4;

	ret = platform_add_devices(adam_sdhci_devices, ARRAY_SIZE(adam_sdhci_devices));
	adam_wifi_init();
	return ret;

}
