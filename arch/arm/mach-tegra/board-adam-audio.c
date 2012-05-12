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
/*#define ALC5623_IS_MASTER*/
 
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
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>
#include <asm/io.h>

#include <mach/tegra_alc5623_pdata.h>
#include <mach/io.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/gpio.h>
#include <mach/spdif.h>
#include <mach/audio.h>  

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

#ifdef USE_ORG_DAS

#include <mach/tegra_das.h>

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

static struct resource das_resource[] = {
	[0] = {
		.start	= TEGRA_APB_MISC_BASE,
		.end	= TEGRA_APB_MISC_BASE + TEGRA_APB_MISC_SIZE - 1,
		.flags	= IORESOURCE_MEM
	}
};

static struct platform_device das_device = {
	.name		= "tegra_das",
	.id		= -1,
	.resource	= das_resource,
	.num_resources	= ARRAY_SIZE(das_resource),
	.dev = {
		.platform_data = &tegra_das_pdata,
	}
}; 
#endif

static struct tegra_audio_platform_data tegra_spdif_pdata = {
	.dma_on			= true,  /* use dma by default */
	.mask			= TEGRA_AUDIO_ENABLE_TX | TEGRA_AUDIO_ENABLE_RX,
	.stereo_capture = true,
};

static struct tegra_audio_platform_data tegra_audio_pdata[] = {
	/* For I2S1 - Hifi */
	[0] = {
#ifdef ALC5623_IS_MASTER
		.i2s_master		= false,	/* CODEC is master for audio */
		.dma_on			= true,  	/* use dma by default */
		.i2s_clk_rate 	= 2822400,
		.dap_clk	  	= "cdev1",
		.audio_sync_clk = "audio_2x",
		.mode			= I2S_BIT_FORMAT_I2S,
		.fifo_fmt		= I2S_FIFO_16_LSB,
		.bit_size		= I2S_BIT_SIZE_16,
#else
		.i2s_master		= true,		/* CODEC is slave for audio */
		.dma_on			= true,  	/* use dma by default */
#ifdef ADAM_48KHZ_AUDIO						
		.i2s_master_clk = 48000,
		.i2s_clk_rate 	= 12288000,
#else
		.i2s_master_clk = 44100,
		.i2s_clk_rate 	= 11289600,
#endif
		.dap_clk	  	= "cdev1",
		.audio_sync_clk = "audio_2x",
		.mode			= I2S_BIT_FORMAT_I2S,
		.fifo_fmt		= I2S_FIFO_PACKED,
		.bit_size		= I2S_BIT_SIZE_16,
		.i2s_bus_width	= 32,
#endif
		.mask			= TEGRA_AUDIO_ENABLE_TX | TEGRA_AUDIO_ENABLE_RX,
		.stereo_capture = true,
	},
	/* For I2S2 - Bluetooth */
	[1] = {
		.i2s_master		= false,	/* bluetooth is master always */
		.dma_on			= true,  /* use dma by default */
		.i2s_master_clk = 8000,
		.dsp_master_clk = 8000,
		.i2s_clk_rate	= 2000000,
		.dap_clk		= "cdev1",
		.audio_sync_clk = "audio_2x",
		.mode			= I2S_BIT_FORMAT_DSP,
		.fifo_fmt		= I2S_FIFO_16_LSB,
		.bit_size		= I2S_BIT_SIZE_16,
		.i2s_bus_width 	= 32,
		.dsp_bus_width 	= 16,
		.mask			= TEGRA_AUDIO_ENABLE_TX | TEGRA_AUDIO_ENABLE_RX,
		.stereo_capture = true,
	}
}; 

static struct alc5623_platform_data adam_alc5623_pdata = {
	.add_ctrl	= 0xD300,
	.jack_det_ctrl	= 0,
	.avdd_mv		= 3300,	/* Analog vdd in millivolts */

	.mic1bias_mv		= 2475,	/* MIC1 bias voltage */
	.mic2bias_mv		= 2475,	/* MIC2 bias voltage */
	.mic1boost_db		= 30,	/* MIC1 gain boost */
	.mic2boost_db		= 30,	/* MIC2 gain boost */

	.default_is_mic2 	= false,	/* Adam uses MIC1 as the default capture source */

};

static struct i2c_board_info __initdata adam_i2c_bus0_board_info[] = {
	{
		I2C_BOARD_INFO("alc5623", 0x1a),
		.platform_data = &adam_alc5623_pdata,
	},
};


static struct tegra_alc5623_platform_data adam_audio_pdata = {
        .gpio_spkr_en           = -2,
        .gpio_hp_det            = ADAM_HP_DETECT,
	.gpio_int_mic_en 	= ADAM_INT_MIC_EN,
#ifndef USE_ORG_DAS	
	.hifi_codec_datafmt = SND_SOC_DAIFMT_I2S,	/* HiFi codec data format */
#ifdef ALC5624_IS_MASTER
	.hifi_codec_master  = true,					/* If Hifi codec is master */
#else
	.hifi_codec_master  = false,				/* If Hifi codec is master */
#endif
	.bt_codec_datafmt   = SND_SOC_DAIFMT_DSP_A,	/* Bluetooth codec data format */
	.bt_codec_master    = true,					/* If bt codec is master */
#endif

};

static struct platform_device tegra_generic_codec = {
	.name = "tegra-generic-codec",
	.id   = -1,
};

static struct platform_device adam_audio_device = {
	.name = "tegra-snd-alc5623",
	.id   = 0,
        .dev    = {
                .platform_data  = &adam_audio_pdata,
        },

};

static struct platform_device *adam_i2s_devices[] __initdata = {
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&tegra_spdif_device,
#ifdef USE_ORG_DAS
	&das_device,
#else
	&tegra_das_device,
#endif
	&spdif_dit_device,
	&tegra_pcm_device,
	&tegra_generic_codec,
	&adam_audio_device, /* this must come last, as we need the DAS to be initialized to access the codec registers ! */
};


int  __init adam_audio_register_devices(void)
{
        pr_info("%s++", __func__);

	int ret;

	/* Patch in the platform data */
	tegra_i2s_device1.dev.platform_data = &tegra_audio_pdata[0];
	tegra_i2s_device2.dev.platform_data = &tegra_audio_pdata[1];
	tegra_spdif_device.dev.platform_data = &tegra_spdif_pdata;

        ret = i2c_register_board_info(0, adam_i2c_bus0_board_info,
                ARRAY_SIZE(adam_i2c_bus0_board_info));
        if (ret)
                return ret;

        return platform_add_devices(adam_i2s_devices, ARRAY_SIZE(adam_i2s_devices));
}

