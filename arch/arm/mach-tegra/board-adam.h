/*
 * arch/arm/mach-tegra/board-adam.h
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

#ifndef _MACH_TEGRA_BOARD_ADAM_H
#define _MACH_TEGRA_BOARD_ADAM_H

#define TPS6586X_INT_BASE	TEGRA_NR_IRQS

#define ADAM_BT_RESET 		TEGRA_GPIO_PU0 	/* 0= reset asserted */

/* GPS and Magnetic sensor share the same enabling IO line */
#define ADAM_GPSMAG_DISABLE  	TEGRA_GPIO_PV3 	/* 0= disabled */
#define ADAM_3G_DISABLE		TEGRA_GPIO_PJ2 /* 0 = disabled */
#define ADAM_KEY_VOLUMEUP 	TEGRA_GPIO_PD4 	/* 0=pressed */
#define ADAM_KEY_VOLUMEDOWN 	TEGRA_GPIO_PV4 	/* 0=pressed */
#define ADAM_KEY_POWER 		TEGRA_GPIO_PV2 	/* 0=pressed */
#define ADAM_KEY_BACK		TEGRA_GPIO_PH0	/* 0=pressed */

/* #define ADAM_EMC_SAMSUNG		*/
/* #define ADAM_EMC_ELPIDA50NM	*/
/* #define ADAM_EMC_ELPIDA40NM	*/

#define ADAM_CAMERA_POWER 	TEGRA_GPIO_PBB5 /* 1=powered on */
#define ADAM_CAMERA_ROTATION	TEGRA_GPIO_PX7

#define ADAM_NAND_WPN		TEGRA_GPIO_PC7	/* NAND flash write protect: 0=writeprotected */

#define ADAM_BL_ENB			TEGRA_GPIO_PD3
#define ADAM_LVDS_SHUTDOWN	TEGRA_GPIO_PB2 // Is this right?
#define ADAM_EN_VDD_PANEL	TEGRA_GPIO_PC6 
#define ADAM_BL_VDD			TEGRA_GPIO_PW0
#define ADAM_BL_PWM			TEGRA_GPIO_PU3 /* PWM */
#define ADAM_HDMI_ENB		TEGRA_GPIO_PV5 /* unconfirmed */ // Does adam have HDMI enbl?
#define ADAM_HDMI_HPD		TEGRA_GPIO_PN7 /* 1=HDMI plug detected */

#define ADAM_BL_PWM_ID		0				/* PWM0 controls backlight */

#define ADAM_FB_PAGES		2				/* At least, 2 video pages */
#define ADAM_FB_HDMI_PAGES	2				/* At least, 2 video pages for HDMI */

// Adam memory is 2xSZ_512M
#define ADAM_MEM_SIZE 		SZ_512M			/* Total memory */
#define ADAM_MEM_BANKS		2

#define ADAM_GPU_MEM_SIZE 		SZ_128M		/* Memory reserved for GPU */
//#define ADAM_GPU_MEM_SIZE 	SZ_64M			/* Memory reserved for GPU */

#define ADAM_FB1_MEM_SIZE 	SZ_8M			/* Memory reserved for Framebuffer 1: LCD */
#define ADAM_FB2_MEM_SIZE 	SZ_8M			/* Memory reserved for Framebuffer 2: HDMI out */

#define DYNAMIC_GPU_MEM 1						/* use dynamic memory for GPU */

/*#define ADAM_48KHZ_AUDIO*/ /* <- define this if you want 48khz audio sampling rate instead of 44100Hz */


// TPS6586x GPIOs as registered 
#define PMU_GPIO_BASE		(TEGRA_NR_GPIOS) 
#define PMU_GPIO0 		(PMU_GPIO_BASE)
#define PMU_GPIO1 		(PMU_GPIO_BASE + 1) 
#define PMU_GPIO2 		(PMU_GPIO_BASE + 2)
#define PMU_GPIO3 		(PMU_GPIO_BASE + 3)

#define ALC5623_GPIO_BASE	(TEGRA_NR_GPIOS + 16)
#define ALC5623_GP0		(ALC5623_GPIO_BASE)

#define PMU_IRQ_BASE		(TEGRA_NR_IRQS)
#define PMU_IRQ_RTC_ALM1 	(TPS6586X_INT_BASE + TPS6586X_INT_RTC_ALM1)

#define	ADAM_ENABLE_VDD_VID	TEGRA_GPIO_PD1	/* 1=enabled.  Powers HDMI. Wait 500uS to let it stabilize before returning */

// TODO: Find whether there are any definitions for these?
/*#define ADAM_SDIO0_CD		TEGRA_GPIO_PI5
#define ADAM_SDIO0_POWER	TEGRA_GPIO_PD0*/	/* SDIO0 and SDIO2 power */

#define ADAM_SDHC_CD		TEGRA_GPIO_PI5
#define ADAM_SDHC_WP		-1	/*1=Write Protected */
#define ADAM_SDHC_POWER	TEGRA_GPIO_PD0

#define ADAM_TS_IRQ		TEGRA_GPIO_PJ7
#define ADAM_TS_RESET	TEGRA_GPIO_PH1
#define ADAM_TS_POWER	TEGRA_GPIO_PK2
//#define ADAM_TS_DISABLE	TEGRA_GPIO_PAA6 /* 0=enabled */

//#define ADAM_FB_NONROTATE TEGRA_GPIO_PH1 /*1 = screen rotation locked */

#define ADAM_WLAN_POWER 	TEGRA_GPIO_PK5
#define ADAM_WLAN_RESET 	TEGRA_GPIO_PK6

#define ADAM_BT_RST		TEGRA_GPIO_PU0

#define ADAM_LOW_BATT	TEGRA_GPIO_PW3 /*(0=low battery)*/
#define ADAM_IN_S3		TEGRA_GPIO_PAA7 /*1 = in S3 */

#define ADAM_USB0_VBUS		TEGRA_GPIO_PB0		/* 1= VBUS usb0 */
#define ADAM_USB1_RESET		TEGRA_GPIO_PV1	/* 0= reset */

#define ADAM_HP_DETECT	TEGRA_GPIO_PW2 	/* HeadPhone detect for audio codec: 1=Hedphone plugged */

#define ADAM_NVEC_REQ	TEGRA_GPIO_PD0	/* Set to 0 to send a command to the NVidia Embedded controller */
#define ADAM_NVEC_I2C_ADDR 0x8a 			/* I2C address of Tegra, when acting as I2C slave */

#define ADAM_WAKE_KEY_POWER  TEGRA_WAKE_GPIO_PV2
#define ADAM_WAKE_KEY_RESUME TEGRA_WAKE_GPIO_PV2

/* The switch used to indicate rotation lock */
//#define SW_ROTATION_LOCK 	(SW_MAX-1)

extern void adam_gps_mag_poweron(void);
extern void adam_gps_mag_poweroff(void);
extern void adam_gps_mag_init(void);

extern void adam_wifi_set_cd(int val);

extern void adam_init_emc(void);
extern void adam_pinmux_init(void);
extern void adam_clks_init(void);

extern int adam_usb_register_devices(void);
extern int adam_audio_register_devices(void);
extern int adam_jack_register_devices(void);
extern int adam_gpu_register_devices(void);
extern int adam_uart_register_devices(void);
extern int adam_spi_register_devices(void);
extern int adam_aes_register_devices(void);
extern int adam_wdt_register_devices(void);
extern int adam_i2c_register_devices(void);
extern int adam_power_register_devices(void);
extern int adam_keyboard_register_devices(void);
extern int adam_touch_register_devices(void);
extern int adam_sdhci_register_devices(void);
extern int adam_sensors_register_devices(void);
extern int adam_wlan_pm_register_devices(void);
extern int adam_gps_pm_register_devices(void);
extern int adam_gsm_pm_register_devices(void);
extern int adam_bt_pm_register_devices(void);
extern int adam_nand_register_devices(void);
extern int adam_camera_register_devices(void);

#endif

