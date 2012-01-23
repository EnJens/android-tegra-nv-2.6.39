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
	/* always on clocks */
	
	/* 32khz system clock */
	{ "clk_32k",	NULL,				32768,	true},		/* always on */

	/* Master clock */
	{ "clk_m",		NULL,		 	 		0,	true},	 	/* must be always on - Frequency will be autodetected */
	
	/* pll_s generates the master clock */
	{ "pll_s",		"clk_32k",		 12000000,	true},		/* must be always on */
	
	/* pll_p is used as system clock - and for ulpi */
	{ "pll_p",		"clk_m",		216000000,	true},		/* must be always on */
	{ "pll_p_out1",	"pll_p",		 28800000,	true},		/* must be always on - audio clocks ...*/
//	{ "pll_p_out2",	"pll_p",		 48000000,	true},		/* must be always on - usb perhaps ? unused ?*/
	{ "pll_p_out2",	"pll_p",		 108000000,	true},		/* must be always on - usb perhaps ? unused ?*/
	{ "pll_p_out3",	"pll_p",		 72000000,	true},		/* must be always on - i2c, camera */
	{ "pll_p_out4",	"pll_p",		 26000000,	true},		/* must be always on - USB ulpi */
	
	/* pll_m is used as memory clock - bootloader also uses it this way */
	{ "pll_m",		"clk_m",		666000000,	true},		/* always on - memory clocks */	
//	{ "pll_m_out1",	"pll_m",		 26000000,	true},		/* always on - unused ?*/
	{ "pll_m_out1",	"pll_m",		 222000000,	true},		/* always on - unused ?*/
	{ "emc",		"pll_m",		666000000,	true},		/* always on */

	/* pll_c is used as graphics clock and system clock */
	{ "pll_c",		"clk_m",		600000000,	true},		/* always on - graphics and camera clocks */
//	{ "pll_c_out1",	"pll_c",		108000000,	true},		/* must be always on - system clock */
	{ "pll_c_out1",	"pll_c",		171428571,	true},		/* must be always on - system clock */

	{ "sclk",		"pll_p_out2",	108000000,	true},		/* must be always on */
	{ "avp.sclk",   	NULL,		108000000,      false},         /* must be always on */
        { "cop",                "sclk",                 108000000,      false},         /* must be always on */
	{ "hclk",		"sclk",			108000000,	true},		/* must be always on */
//	{ "pclk",		"hclk",			 54000000,	true},		/* must be always on */
	{ "pclk",		"hclk",			108000000,	true},		/* must be always on */

	/* pll_a and pll_a_out0 are clock sources for audio interfaces */
#ifdef ALC5623_IS_MASTER
	{ "pll_a",		"pll_p_out1",	 73728000,	true},		/* always on - audio clocks */
	{ "pll_a_out0",	"pll_a",		 18432000,	true},		/* always on - i2s audio */
#else
#       ifdef ADAM_48KHZ_AUDIO
        { "pll_a",              "pll_p_out1",    73728000,      true},          /* always on - audio clocks */
        { "pll_a_out0", 	"pll_a",         12288000,      true},          /* always on - i2s audio */
#       else
        { "pll_a",              "pll_p_out1",    56448000,      true},          /* always on - audio clocks */
        { "pll_a_out0", 	"pll_a",         11289600,      true},          /* always on - i2s audio */
#       endif
#endif

	/* pll_d and pll_d_out0 are clock sources for HDMI output */
	{ "pll_d",		"clk_m",		  5000000,	true},		/* hdmi clock */
//	{ "pll_d_out0", "pll_d",    	  5000000,  true},		/* hdmi clock */
	{ "pll_d_out0", "pll_d",    	  2500000,  true},		/* hdmi clock */

	{ "clk_d",		"clk_m",		 24000000,	true},

	/* pll_u is a clock source for the USB bus */
	{ "pll_u",  	"clk_m",    	480000000,  false},		/* USB ulpi clock */

        /* pll_x */
        { "pll_x",      "clk_m",        760000000,  true},              /* */
        { "cclk",               "pll_x",                760000000,  true},
        { "cpu",                "cclk",                 760000000,  true},
	
	/* Peripherals - Always on */
	{ "csite",		"pll_p",		144000000,	true},		/* csite - coresite */ /* always on */
	{ "timer",		"clk_m",		 12000000,	true},		/* timer */ /* always on - no init req */
	{ "rtc",		"clk_32k",			32768,	true},		/* rtc-tegra : must be always on */
	{ "kfuse",		"clk_m",		 12000000,	true},		/* kfuse-tegra */ /* always on - no init req */

	/* Peripherals - Turned on demand */
//	{ "3d",     	"pll_c",    	300000000,  false},		/* tegra_grhost, gr3d */
	{ "3d",     	"pll_c",    	333000000,  false},		/* tegra_grhost, gr3d */
	{ "2d",     	"pll_c",    	300000000,  false},		/* tegra_grhost, gr2d */
	{ "epp",    	"pll_c",    	300000000, 	false}, 	/* tegra_grhost */	
//	{ "mpe",		"pll_c",		300000000,	false},		/* tegra_grhost */	
	{ "mpe",		"pll_c",		266400000,	false},		/* tegra_grhost */	
//	{ "host1x",		"pll_p",		144000000,	false},		/* tegra_grhost */
	{ "host1x",		"pll_p",		108000000,	false},		/* tegra_grhost */
	
//	{ "vi",     	"pll_c",   		150000000,  true},		/* tegra_camera */
	{ "vi",     	"pll_c",   		111000000,  true},		/* tegra_camera */
//	{ "vi_sensor",	"pll_c",		150000000,	true},		/* tegra_camera */
	{ "vi_sensor",	"pll_c",		111000000,	true},		/* tegra_camera */
	{ "csi",		"pll_p_out3",	 72000000,	true},		/* tegra_camera */
	{ "isp",		"clk_m",		 12000000,	true},		/* tegra_camera */
	{ "csus",		"clk_m",		 12000000,	true},		/* tegra_camera */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	{ "pex",		"clk_m",		 12000000,	false},		/* pcie controller */
	{ "afi",		"clk_m",		 12000000,	false},		/* pcie controller */
	{ "pcie_xclk",	"clk_m",		 12000000,	false},		/* pcie controller */
#endif
		
#ifdef ALC5623_IS_MASTER		
	{ "i2s1",   	"clk_m",         12000000,  		true},		/* i2s.0 */
	{ "i2s2",		"clk_m",	     12000000,	true},		/* i2s.1 */
	{ "audio", 		"i2s1",          12000000,  	true},
	{ "audio_2x",	"audio",		 24000000,	true},
	{ "spdif_in",	"pll_p",		 36000000,	true},
	{ "spdif_out",  "pll_a_out0",  	  	 6144000,  	true},
#else
#       ifdef ADAM_48KHZ_AUDIO
        { "i2s1",       "pll_a_out0",    	12288000,  	true},             /* i2s.0 */
        { "i2s2",               "pll_a_out0",    12288000,      true},         /* i2s.1 */
        { "audio",              "pll_a_out0",    12288000,  	true},
        { "audio_2x",   "audio",                 24576000,      true},
        { "spdif_in",   "pll_p",                 36000000,      true},
        { "spdif_out",  "pll_a_out0",     	6144000,  	true},
#       else
        { "i2s1",       "pll_a_out0",     	2822400,  	true},             /* i2s.0 */
        { "i2s2",               "pll_a_out0",    11289600,      true},         /* i2s.1 */
        { "audio",              "pll_a_out0",    11289600,  	true},
        { "audio_2x",   "audio",                 22579200,      true},
        { "spdif_in",   "pll_p",                 36000000,      true},
        { "spdif_out",  "pll_a_out0",     	5644800,  	true},
#       endif
#endif
	
	/* cdev[1-2] take the configuration (clock parents) from the pinmux config, 
	   That is why we are setting it to NULL */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#define CDEV1 "cdev1"
#define CDEV2 "cdev2"
#else
#define CDEV1 "clk_dev1"
#define CDEV2 "clk_dev2"
#endif
#       ifdef ADAM_48KHZ_AUDIO
//        { CDEV1,   NULL /*"pll_a_out0"*/,12288000,  false},             /* used as audio CODEC MCLK */
        { CDEV1,   NULL /*"pll_a_out0"*/,0,  true},             /* used as audio CODEC MCLK */
#       else
//        { CDEV1,   NULL /*"pll_a_out0"*/,11289600,  false},             /* used as audio CODEC MCLK */
        { CDEV1,   NULL /*"pll_a_out0"*/,0,  true},             /* used as audio CODEC MCLK */
#       endif
//	{ CDEV2,   NULL,	26000000,  false}, 	/* probably used as USB clock - perhaps 24mhz ?*/	
	{ CDEV2,   NULL,	0,  false}, 	/* probably used as USB clock - perhaps 24mhz ?*/	

	
	{ "i2c1_i2c",	"pll_p_out3",	 72000000,	false},		/* tegra-i2c.0 */
	{ "i2c2_i2c",	"pll_p_out3",	 72000000,	false},		/* tegra-i2c.1 */
	{ "i2c3_i2c",	"pll_p_out3",	 72000000,	false},		/* tegra-i2c.2 */
	{ "dvc_i2c",	"pll_p_out3",	 72000000,	false},		/* tegra-i2c.3 */
	{ "i2c1",		"clk_m",		  3000000,	false},		/* tegra-i2c.0 */
	{ "i2c2",		"clk_m",		  3000000,	false},		/* tegra-i2c.1 */
	{ "i2c3",		"clk_m",		  3000000,	false},		/* tegra-i2c.2 */
	{ "dvc",		"clk_m",		  3000000,	false},		/* tegra-i2c.3 */

	{ "apbdma",		"pclk",			108000000,	true}, 	/* tegra-dma */
	{ "uarta",		"pll_p",		216000000,	false},		/* tegra_uart.0 uart.0 */
	{ "uartb", 	 	"pll_p",  	  	216000000,	false},		/* tegra_uart.1 uart.1 */
	{ "uartc",		"pll_p",		216000000,	false},		/* tegra_uart.2 uart.2 */
	{ "uartd",		"pll_p",		216000000,	false},		/* tegra_uart.3 uart.3 */
	{ "uarte",		"pll_p",		216000000,	false},		/* tegra_uart.4 uart.4 */

	{ "disp1",  	"pll_p",    	216000000, 	false},		/* tegradc.0 */
	{ "disp2",  	"pll_p",    	216000000, 	false},		/* tegradc.1 */	
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)	
	{ "dsia",		"pll_d",		  5000000,	false},		/* tegra_dc.0, tegra_dc.1 */
#else
	{ "dsia",		"pll_d_out0",	  2500000,	false},		/* tegra_dc.0, tegra_dc.1 - bug on kernel 2.6.36*/
#endif
	{ "hdmi",		"clk_m",		 12000000,	false},		/* tegra_dc.0, tegra_dc.1 */
	
	{ "spi",		"clk_m",		 12000000,	false},
	{ "xio",		"clk_m",		 12000000,	false},
	{ "twc",		"clk_m",		 12000000,	false},
	
	{ "sbc1",		"clk_m",		 12000000,	false}, 	/* tegra_spi_slave.0 */
	{ "sbc2",		"clk_m",		 12000000,	false}, 	/* tegra_spi_slave.1 */
	{ "sbc3",		"clk_m",		 12000000,	false}, 	/* tegra_spi_slave.2 */
	{ "sbc4",		"clk_m",		 12000000,	false}, 	/* tegra_spi_slave.3 */
	
	{ "ide",		"clk_m",		 12000000,	false},
	
	{ "ndflash",	"pll_p",		108000000,	true},		/* tegra_nand -> should start disabled, but if we do, then nand stops working */
		
	{ "vfir",		"clk_m",		 12000000,	false},

	{ "sdmmc1",		"pll_p",		 48000000,	false},		/* sdhci-tegra.0 */
	{ "sdmmc2",		"pll_p",		 48000000,	false},		/* sdhci-tegra.1 */
	{ "sdmmc3",		"pll_p",		 48000000,	false},		/* sdhci-tegra.2 */
	{ "sdmmc4",		"pll_p",		 48000000,	false},		/* sdhci-tegra.3 */

	{ "la",			"clk_m",		 12000000,	false},			

	{ "owr",		"clk_m",		 12000000,	false},		/* tegra_w1 */
	
	{ "vcp",		"clk_m",		 12000000,	false},		/* tegra_avp */	
	{ "bsea",		"clk_m",		 12000000,	false},		/* tegra_avp */	
	{ "vde",		"pll_p",		216000000,	false},		/* tegra-avp */
	
	{ "bsev",		"clk_m",		 12000000,	false},		/* tegra_aes */	

	{ "nor",		"clk_m",		 12000000,	false},			
	{ "mipi",		"clk_m",		 12000000,	false},			
	{ "cve",		"clk_m",		 12000000,	false},			
	{ "tvo",		"clk_m",		 12000000,	false},			
	{ "tvdac",		"clk_m",		 12000000,	false},			

	{ "usbd",		"clk_m",		 12000000,	false},		/* fsl-tegra-udc , utmip-pad , tegra_ehci.0 , tegra_otg */
	{ "usb2",		"clk_m",		 12000000,	false},		/* tegra_ehci.1 */
	{ "usb3",		"clk_m",		 12000000,	false},		/* tegra_ehci.2 */
	
	{ "pwm",    	"clk_m",   		 12000000,  false},		/* tegra-pwm.0 tegra-pwm.1 tegra-pwm.2 tegra-pwm.3*/
	
	{ "kbc",		"clk_32k",			32768,	false},		/* tegra-kbc */
	{ "blink",		"clk_32k",			32768,	false},		/* used for bluetooth */

	{ NULL,		NULL,		0,		0},
};

void __init adam_clks_init(void)
{
	tegra_clk_init_from_table(adam_clk_init_table);
}
