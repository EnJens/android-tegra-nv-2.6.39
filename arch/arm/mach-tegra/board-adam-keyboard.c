/* OK */
/*
 * arch/arm/mach-tegra/board-adam-keyboard.c
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

#include <linux/platform_device.h>
#include <linux/input.h>

#include <linux/gpio_keys.h>
//#include <linux/gpio_shortlong_key.h>

#include <linux/gpio.h>
#include <asm/mach-types.h>

#include "board-adam.h"
#include "gpio-names.h"

static struct gpio_keys_button adam_keys[] = {
	[0] = {
		.gpio = ADAM_KEY_VOLUMEUP,
		.active_low = true,
		.debounce_interval = 10,
		.wakeup = false,		
		.code = KEY_VOLUMEUP,
		.type = EV_KEY,		
		.desc = "volume up",
	},
	[1] = {
		.gpio = ADAM_KEY_VOLUMEDOWN,
		.active_low = true,
		.debounce_interval = 10,
		.wakeup = false,		
		.code = KEY_VOLUMEDOWN,
		.type = EV_KEY,		
		.desc = "volume down",
	},
	[2] = {
		.gpio = ADAM_KEY_POWER,
		.active_low = true,
		.debounce_interval = 100,
		.wakeup = true,		
		.code = KEY_POWER,
		.type = EV_KEY,		
		.desc = "power",
	},
	[3] = {
		.gpio = ADAM_KEY_BACK,
		.active_low = true,
		.debounce_interval = 10,
		.wakeup = true,		
		.code = KEY_BACK,
		.type = EV_KEY,		
		.desc = "back",
	},
};


static struct gpio_keys_platform_data adam_keys_platform_data = {
	.buttons 	= adam_keys,
	.nbuttons 	= ARRAY_SIZE(adam_keys),
	.rep		= true, /* auto repeat enabled */
};

static struct platform_device adam_keys_device = {
	.name 		= "gpio-keys",
	.id 		= 0,
	.dev		= {
		.platform_data = &adam_keys_platform_data,
	},
};

static struct platform_device *adam_pmu_devices[] __initdata = {
	&adam_keys_device,
};

/* Register all keyboard devices */
int __init adam_keyboard_register_devices(void)
{
	return platform_add_devices(adam_pmu_devices, ARRAY_SIZE(adam_pmu_devices));
}

