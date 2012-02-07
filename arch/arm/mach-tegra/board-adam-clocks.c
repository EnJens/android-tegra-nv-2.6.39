/*
 * arch/arm/mach-tegra/board-adam-clocks.c
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

#include <linux/console.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/fsl_devices.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/pda_power.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/i2c-tegra.h>
#include <linux/memblock.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>

#include <mach/io.h>
#include <mach/w1.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/nand.h>
#include <mach/sdhci.h>
#include <mach/gpio.h>
#include <mach/clk.h>
#include <mach/usb_phy.h>
//#include <mach/tegra2_i2s.h>
#include <mach/system.h>
#include <mach/nvmap.h>

#include "board.h"
#include "board-adam.h"
#include "clock.h"
#include "gpio-names.h"
#include "devices.h"

/* Be careful here: Most clocks have restrictions on parent and on
   divider/multiplier ratios. Check tegra2clocks.c before modifying
   this table ! */

static __initdata struct tegra_clk_init_table adam_clk_init_table[] = {
	/* name			parent				rate	enabled */
	{ "pll_m",		NULL,			0,		false},
	/* 32khz system clock */
	{ "clk_32k",		NULL,			32768,		true},		/* always on */
	{ "rtc",		"clk_32k",			32768,		true},		/* rtc-tegra : must be always on */
	{ "blink",		"clk_32k",			32768,		false},		/* used for bluetooth */
	/* Master clock */
	{ "sdmmc2",		"pll_p",		48000000,	false},		/* sdhci-tegra.1 */
	{ "pwm",		"clk_m",		12000000,	true},		/* tegra-pwm.0 tegra-pwm.1 tegra-pwm.2 tegra-pwm.3*/
        { "i2s1", 		"pll_a_out0",		2822400,	true}, /* i2s.0 */
        { "i2s2",		"pll_a_out0",		11289600,	true}, /* i2s.1 */
	{ "cdev1",		NULL,			0,		true},
	{ "uartc",		"pll_p",		216000000,	false},    /* tegra_uart.2 uart.0 */
	{  NULL,		NULL,			0,		0},
};


void __init adam_clks_init(void)
{
	tegra_clk_init_from_table(adam_clk_init_table);
}
