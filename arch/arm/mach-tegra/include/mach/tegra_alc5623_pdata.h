/*
 * arch/arm/mach-tegra/include/mach/tegra_alc5623_pdata.h
 *
 * Author: Jason Stern
 *
 * Based on code copyright/by:
 *
 * Copyright 2011 NVIDIA, Inc.
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
#include "../../board-adam.h"
 
struct tegra_alc5623_platform_data {
	int gpio_spkr_en;
	int gpio_hp_det;
	int gpio_int_mic_en;
#ifndef USE_ORG_DAS
	int  hifi_codec_datafmt;/* HiFi codec data format */
	bool hifi_codec_master;/* If Hifi codec is master */
	int  bt_codec_datafmt;	/* Bluetooth codec data format */
	bool bt_codec_master;	/* If bt codec is master */
#endif	
};
