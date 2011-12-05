/*
 * include/linux/i2c/at168_ts.h
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 * Copyright (c) 2011, Jens Andersen <jens.andersen@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LINUX_I2C_AT168_TS_H
#define _LINUX_I2C_AT168_TS_H

struct device;

struct at168_i2c_ts_platform_data {
	int gpio_reset;
	int gpio_power;
};

/* AT168 I2C registers */
#define AT168_TOUCH_NUM			0x00
#define AT168_TOUCH_OLD_NUM			0x01
#define AT168_POS_X0_LO				0x02
#define AT168_POS_X0_HI					0x03
#define AT168_POS_Y0_LO				0x04 
#define AT168_POS_Y0_HI					0x05
#define AT168_POS_X1_LO				0x06 
#define AT168_POS_X1_HI					0x07 
#define AT168_POS_Y1_LO				0x08 
#define AT168_POS_Y1_HI					0x09 
#define AT168_X1_W					0x0a
#define AT168_Y1_W						0x0b
#define AT168_X2_W					0x0c
#define AT168_Y2_W						0x0d
#define AT168_X1_Z					0x0e
#define AT168_Y1_Z						0x0f
#define AT168_X2_Z					0x10
#define AT168_Y2_Z						0x11
#define AT168_POS_PRESSURE_LO		0x12
#define AT168_POS_PRESSURE_HI			0x13
#define AT168_POWER_MODE			0x14
#define AT168_INIT_MODE					0x15

#define AT168_XMAX_LO				0x1a
#define AT168_XMAX_HI					0x1b
#define AT168_YMAX_LO				0x1c
#define AT168_YMAX_HI					0x1d
#define AT168_VERSION_TS_SUPPLIER	0x1e
#define AT168_VERSION_FW				0x1f
#define AT168_VERSION_BOOTLOADER	0x20
#define AT168_VERSION_PROTOCOL			0x21

#define AT168_SINTEK_BASELINE_X1_VALUE				0x7d  //125
#define AT168_SINTEK_BASELINE_X2_VALUE				0x3d  //61
#define AT168_SINTEK_BASELINE_Y_VALUE				0x4d  //77

#define AT168_SINTEK_CALIBRATERESULT_X1_VALUE				0x7d  //125
#define AT168_SINTEK_CALIBRATERESULT_X2_VALUE				0x3d  //61
#define AT168_SINTEK_CALIBRATERESULT_Y_VALUE				0x4d  //77

#define AT168_CANDO_BASELINE_COMMAND				0xc7
#define AT168_CANDO_CALIBRATERESULT_COMMAND		0xc8

#define AT168_SPECOP				0x37

#define AT168_INTERNAL_ENABLE				0xc2 //194

/* AT168 registers Init value*/
#define AT168_POWER_MODE_VALUE	0xa4
#define AT168_INIT_MODE_VALUE		0x0a
#define AT168_SPECOP_DEFAULT_VALUE		0x00
#define AT168_SPECOP_CALIBRATION_VALUE		0x03

#define AT168_INTERNAL_ENABLE_VALUE		0x01
#define AT168_INTERNAL_DISABLE_VALUE		0x00

#endif
