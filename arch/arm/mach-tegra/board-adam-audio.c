/*
 * arch/arm/mach-tegra/board-adam-audio.c
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

/* All configurations related to audio */
/*#define ALC5623_IS_MASTER */
 
#include <linux/console.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <sound/alc5623.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>
#include <asm/io.h>

#include <mach/io.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/gpio.h>
#include <mach/tegra2_i2s.h>
#include <mach/spdif.h>
#include <mach/audio.h>  
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
#include <mach/tegra_das.h>
#endif

#include <mach/system.h>

#include "board.h"
#include "board-adam.h"
#include "gpio-names.h"
#include "devices.h"

/* Default music path: I2S1(DAC1)<->Dap1<->HifiCodec
   Bluetooth to codec: I2S2(DAC2)<->Dap4<->Bluetooth
*/
/* For Adam, 
	Codec is ALC5623
	Codec I2C Address = 0x34(includes R/W bit), i2c #0
	Codec MCLK = APxx DAP_MCLK1
*/

#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
static struct tegra_das_platform_data tegra_das_pdata = {
	.dap_clk = "clk_dev1",
	.tegra_dap_port_info_table = {
		/* I2S1 <--> DAC1 <--> DAP1 <--> Hifi Codec */
		[0] = {
			.dac_port = tegra_das_port_i2s1,
			.dap_port = tegra_das_port_dap1,
			.codec_type = tegra_audio_codec_type_hifi,
			.device_property = {
				.num_channels = 2,
				.bits_per_sample = 16,
#ifdef ADAM_48KHZ_AUDIO
                                .rate = 48000,
#else
                                .rate = 44100,
#endif
				.dac_dap_data_comm_format =
						dac_dap_data_format_all,
			},
		},
		/* I2S2 <--> DAC2 <--> DAP2 <--> Voice Codec */
		[1] = {
			.dac_port = tegra_das_port_i2s2,
			.dap_port = tegra_das_port_dap2,
			.codec_type = tegra_audio_codec_type_voice,
			.device_property = {
				.num_channels = 1,
				.bits_per_sample = 16,
				.rate = 8000,
				.dac_dap_data_comm_format =
						dac_dap_data_format_all,
			},
		},
		/* I2S2 <--> DAC2 <--> DAP3 <--> Baseband Codec */
		[2] = {
			.dac_port = tegra_das_port_i2s2,
			.dap_port = tegra_das_port_dap3,
			.codec_type = tegra_audio_codec_type_baseband,
			.device_property = {
				.num_channels = 1,
				.bits_per_sample = 16,
				.rate = 8000,
				.dac_dap_data_comm_format =
					dac_dap_data_format_dsp,
			},
		},
		/* I2S2 <--> DAC2 <--> DAP4 <--> BT SCO Codec */
		[3] = {
			.dac_port = tegra_das_port_i2s2,
			.dap_port = tegra_das_port_dap4,
			.codec_type = tegra_audio_codec_type_bluetooth,
			.device_property = {
				.num_channels = 1,
				.bits_per_sample = 16,
				.rate = 8000,
				.dac_dap_data_comm_format =
					dac_dap_data_format_dsp,
			},
		},
		[4] = {
			.dac_port = tegra_das_port_none,
			.dap_port = tegra_das_port_none,
			.codec_type = tegra_audio_codec_type_none,
			.device_property = {
				.num_channels = 0,
				.bits_per_sample = 0,
				.rate = 0,
				.dac_dap_data_comm_format = 0,
			},
		},
	},

	.tegra_das_con_table = {
		[0] = {
			.con_id = tegra_das_port_con_id_hifi,
			.num_entries = 2,
			.con_line = { /*src*/            /*dst*/             /* src master */
#ifdef ALC5623_IS_MASTER
				[0] = {tegra_das_port_i2s1, tegra_das_port_dap1, true}, 
				[1] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
#else
				[0] = {tegra_das_port_i2s1, tegra_das_port_dap1, false}, 
				[1] = {tegra_das_port_dap1, tegra_das_port_i2s1, true},
#endif
			},
		},
		[1] = {
			.con_id = tegra_das_port_con_id_bt_codec,
			.num_entries = 4,
			.con_line = {
				[0] = {tegra_das_port_i2s2, tegra_das_port_dap4, true}, /* src is master */
				[1] = {tegra_das_port_dap4, tegra_das_port_i2s2, false},
#ifdef ALC5623_IS_MASTER
				[2] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
				[3] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
#else				
				[2] = {tegra_das_port_i2s1, tegra_das_port_dap1, false},
				[3] = {tegra_das_port_dap1, tegra_das_port_i2s1, true},
#endif				
			},
		},
		[2] = {
			.con_id = tegra_das_port_con_id_voicecall_no_bt,
			.num_entries = 4,
			.con_line = {
				[0] = {tegra_das_port_dap2, tegra_das_port_dap3, true},
				[1] = {tegra_das_port_dap3, tegra_das_port_dap2, false},
#ifdef ALC5623_IS_MASTER
				[2] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
				[3] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
#else
				[2] = {tegra_das_port_i2s1, tegra_das_port_dap1, false},
				[3] = {tegra_das_port_dap1, tegra_das_port_i2s1, true},
#endif
			},
		},
	}
}; 
#endif

static struct tegra_audio_platform_data tegra_spdif_pdata = {
	.dma_on = true,  /* use dma by default */
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
#ifdef ADAM_48KHZ_AUDIO
        .dev_clk_rate = 6144000,
#else
        .dev_clk_rate = 5644800,
#endif
#endif
};

static struct tegra_audio_platform_data tegra_audio_pdata[] = {
	/* For I2S1 - Hifi */
	[0] = {
#ifdef ALC5623_IS_MASTER
		.i2s_master		= false,	/* CODEC is master for audio */
		.dma_on			= true,  	/* use dma by default */
		.i2s_clk_rate 	= 2822400,
		.dap_clk	  	= "clk_dev1",
		.audio_sync_clk = "audio_2x",
		.mode			= I2S_BIT_FORMAT_I2S,
		.fifo_fmt		= I2S_FIFO_16_LSB,
		.bit_size		= I2S_BIT_SIZE_16,
#else
		.i2s_master		= true,		/* CODEC is slave for audio */
		.dma_on			= true,  	/* use dma by default */
#ifdef ADAM_48KHZ_AUDIO
                .i2s_master_clk = 48000,
                .dev_clk_rate   = 12288000,
#else
                .i2s_master_clk = 44100,
                .dev_clk_rate   = 11289600,
#endif
		.dap_clk	  	= "clk_dev1",
		.audio_sync_clk 	= "audio_2x",
		.mode			= I2S_BIT_FORMAT_I2S,
		.fifo_fmt		= I2S_FIFO_PACKED,
		.bit_size		= I2S_BIT_SIZE_16,
		.i2s_bus_width		= 32,
#endif
	},
	/* For I2S2 - Bluetooth */
	[1] = {
		.i2s_master		= true,
		.dma_on			= true,  /* use dma by default */
		.i2s_master_clk 	= 8000,
		.dsp_master_clk 	= 8000,
		.dev_clk_rate		= 2000000,
		.dap_clk		= "clk_dev1",
		.audio_sync_clk		= "audio_2x",
		.mode			= I2S_BIT_FORMAT_DSP,
		.fifo_fmt		= I2S_FIFO_16_LSB,
		.bit_size		= I2S_BIT_SIZE_16,
		.i2s_bus_width 		= 32,
		.dsp_bus_width 		= 16,
	}
}; 

static struct alc5623_platform_data alc5623_pdata = {
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
	.mclk 		= "clk_dev1",
#else
	.mclk 		= "cdev1",
#endif
	.linevdd_mv 	= 5000,	/* Line Vdd in millivolts */
//	.spkvdd_mv 	= 5000,	/* Speaker Vdd in millivolts */
//	.hpvdd_mv 	= 3300,	/* Headphone Vdd in millivolts */
	.add_ctrl	= 0xD300,
	.jack_det_ctrl	= 0,
	.gpio_base	= ALC5623_GPIO_BASE,
};

static struct platform_device alc5623_gpio = {
	.name = "alc5623-gpio",
	.id   = 0,
	.dev = {
		.platform_data = &alc5623_pdata,
	}, 
};

static struct platform_device *adam_gpios[] __initdata = {
	&alc5623_gpio,
};

static struct i2c_board_info __initdata adam_i2c_bus0_board_info[] = {
	{
		I2C_BOARD_INFO("alc5623", 0x1a),
		.platform_data = &alc5623_pdata,
	},
};

static struct platform_device adam_audio_device = {
	.name = "tegra-snd-adam",
	.id   = 0,
};

static struct platform_device spdif_dit_device = {
	.name   = "spdif-dit",
	.id     = -1,
}; 

static struct platform_device *adam_i2s_devices[] __initdata = {
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&spdif_dit_device,
	&tegra_spdif_device,
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
	&tegra_das_device,
#else
	&tegra_pcm_device,
#endif
	&adam_audio_device, /* this must come last, as we need the DAS to be initialized to access the codec registers ! */
};


int __init adam_audio_register_devices(void)
{
	int ret;
	
	/* Patch in the platform data */
	tegra_i2s_device1.dev.platform_data = &tegra_audio_pdata[0];
	tegra_i2s_device2.dev.platform_data = &tegra_audio_pdata[1];
	tegra_spdif_device.dev.platform_data = &tegra_spdif_pdata;
	
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36)
	tegra_das_device.dev.platform_data = &tegra_das_pdata;
#endif

	ret = i2c_register_board_info(0, adam_i2c_bus0_board_info, 
		ARRAY_SIZE(adam_i2c_bus0_board_info)); 
	if (ret)
		return ret;
	
	ret = platform_add_devices(adam_gpios, ARRAY_SIZE(adam_gpios));
	if (ret)
		return ret;

	return platform_add_devices(adam_i2s_devices, ARRAY_SIZE(adam_i2s_devices));
}


