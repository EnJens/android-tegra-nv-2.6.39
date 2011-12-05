/*
 * Copyright (C) 2010 NVIDIA, Inc.
 *               2010 Marc Dietrich <marvin24@gmx.de>
 *               2011 Artem Makhutov <artem@makhutov.org>
 *               2011 Eduardo José Tagle <ejtagle@tutopia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
//#include <linux/regulator/virtual_adj.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/tps6586x.h>
#include <linux/power_supply.h>
//#include <linux/power/nvec_power.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/err.h>
//#include <linux/mfd/nvec.h>

#include <asm/io.h>

#include <mach/io.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/gpio.h>

#include "board-adam.h"
#include "gpio-names.h"
#include "devices.h"


#define PMC_CTRL		0x0
#define PMC_CTRL_INTR_LOW	(1 << 17)

/* Core voltage rail : VDD_CORE -> SM0
*/
static struct regulator_consumer_supply tps658621_sm0_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL)
};

/* CPU voltage rail : VDD_CPU -> SM1
*/
static struct regulator_consumer_supply tps658621_sm1_supply[] = {
	REGULATOR_SUPPLY("vdd_cpu", NULL)
};

/* unused */
static struct regulator_consumer_supply tps658621_sm2_supply[] = {
	REGULATOR_SUPPLY("vdd_sm2", NULL)
};

/* PEX_CLK voltage rail : VDDIO_PEX_CLK -> LDO0
*/
static struct regulator_consumer_supply tps658621_ldo0_supply[] = { /* VDDIO_PEX_CLK */
	REGULATOR_SUPPLY("vdd_pex_clk_1", NULL),
	REGULATOR_SUPPLY("vcore_wifi", NULL),
};

/* PLLA voltage rail : AVDDPLLX_1V2 -> LDO1AVDDPLLX_1V2 -> LDO1
   PLLM voltage rail : AVDDPLLX_1V2 -> LDO1
   PLLP voltage rail : AVDDPLLX_1V2 -> LDO1
   PLLC voltage rail : AVDDPLLX_1V2 -> LDO1
   PLLU voltage rail : AVDD_PLLU -> LDO1
   PLLU1 voltage rail : AVDD_PLLU -> LDO1
   PLLS voltage rail : PLL_S -> LDO1
   PLLX voltage rail : AVDDPLLX -> LDO1
 */
static struct regulator_consumer_supply tps658621_ldo1_supply[] = { /* 1V2 */
	REGULATOR_SUPPLY("pll_a", NULL),
	REGULATOR_SUPPLY("pll_m", NULL),
	REGULATOR_SUPPLY("pll_p", NULL),
	REGULATOR_SUPPLY("pll_c", NULL),
	REGULATOR_SUPPLY("pll_u", NULL),
	REGULATOR_SUPPLY("pll_u1", NULL),
	REGULATOR_SUPPLY("pll_s", NULL),
	REGULATOR_SUPPLY("pll_x", NULL),
};

/* RTC voltage rail : VDD_RTC -> LD02
	>VDD_RTC 
*/
static struct regulator_consumer_supply tps658621_ldo2_supply[] = { 
	REGULATOR_SUPPLY("vdd_rtc", NULL),
	REGULATOR_SUPPLY("vdd_aon", NULL),
};

/* PLL_USB voltage rail : AVDD_USB_PLL -> derived from LDO3 (VDD_3V3)
   USB voltage rail : AVDD_USB -> derived from LDO3 (VDD_3V3)
   NAND voltage rail : VDDIO_NAND_3V3 -> derived from LDO3 (VDD_3V3) (AON domain)
   SDIO voltage rail : VDDIO_SDIO -> derived from LDO3 (VDD_3V3) 
   VI voltage rail : VDDIO_VI -> derived from LDO3 (VDD_3V3)
   LVDS LCD Display : VDD_LVDS (VDD_3V3)
   TMON pwer rail : TMON pwer rail -> LDO3 (VDD_3V3)
*/
static struct regulator_consumer_supply tps658621_ldo3_supply[] = { /* 3V3 */
	REGULATOR_SUPPLY("avdd_usb_pll", NULL),
	REGULATOR_SUPPLY("avdd_usb", NULL),
	REGULATOR_SUPPLY("vddio_nand_3v3", NULL), /* AON? */
	REGULATOR_SUPPLY("sdio", NULL),
	REGULATOR_SUPPLY("vmmc", NULL), 
	REGULATOR_SUPPLY("avdd_lvds", NULL),
	REGULATOR_SUPPLY("tmon0", NULL),
};

/* OSC voltage rail : AVDD_OSC -> LDO4
   SYS IO voltage rail : VDDIO_SYS -> LDO4
   LCD voltage rail : VDDIO_LCD -> (LDO4PG) (AON domain)
   Audio voltage rail : VDDIO_AUDIO -> (LDO4PG) (AON domain)
   DDR voltage rail : VDDIO_DDR -> (LDO4PG) (AON domain)
   UART voltage rail : VDDIO_UART -> (LDO4PG) (AON domain)
   BB voltage rail : VDDIO_BB -> (LDO4PG) (AON domain)
   LVDS LCD Display : VDDIO_LCD (AON:VDD_1V8)
   Power for DDC : VDDIO_LCD (VDD_1V8)
   Power for HDMI Hotplug : HotPlug
   lcd rail (required for crt out) : VDDIO_LCD (VDD_1V8)
   Bluetooth : VDDHOSTIF_BT -> LDO4 (AON:VDD_1V8)
   Wlan : VDDIO_WLAN (AON:VDD_1V8)
*/
static struct regulator_consumer_supply tps658621_ldo4_supply[] = { /* VDD IO VI */
	REGULATOR_SUPPLY("avdd_osc", NULL),       /* AVDD_OSC */
	REGULATOR_SUPPLY("vddio_sys", NULL),
	REGULATOR_SUPPLY("vddio_lcd", NULL),      /* AON? */
	REGULATOR_SUPPLY("vddio_audio", NULL),    /* AON? */
	REGULATOR_SUPPLY("vddio_ddr", NULL),      /* AON? */
	REGULATOR_SUPPLY("vddio_uart", NULL),     /* AON? */
	REGULATOR_SUPPLY("vddio_bb", NULL),       /* AON? */
	REGULATOR_SUPPLY("tmon1.8vs", NULL),
	REGULATOR_SUPPLY("vddhostif_bt", NULL),
	REGULATOR_SUPPLY("wifi3vs", NULL),
	REGULATOR_SUPPLY("vddio_wlan", NULL),
};

/*unused*/
static struct regulator_consumer_supply tps658621_ldo5_supply[] = {
	REGULATOR_SUPPLY("vdd ld5", NULL)
};

/* VDAC voltage rail : AVDD_VDAC -> LDO6
   tvdac rail : VDDIO_VDAC,AVDD_VDAC
 */
static struct regulator_consumer_supply tps658621_ldo6_supply[] = {
	//REGULATOR_SUPPLY("vddio vdac", NULL),
	REGULATOR_SUPPLY("avdd_vdac", NULL)
};

/* HDMI voltage rail : AVDD_HDMI -> LDO7
*/
static struct regulator_consumer_supply tps658621_ldo7_supply[] = {
	REGULATOR_SUPPLY("avdd_hdmi", NULL)
};

/* PLLHD voltage rail (HDMI) : AVDD_HDMI_PLL -> LDO8
*/
static struct regulator_consumer_supply tps658621_ldo8_supply[] = { /* AVDD_HDMI_PLL */
	REGULATOR_SUPPLY("avdd_hdmi_pll", NULL),  //PLLHD 
};

/* DDR_RX voltage rail : VDDIO_RX_DDR(2.7-3.3) -> LDO9
*/
static struct regulator_consumer_supply tps658621_ldo9_supply[] = {
	REGULATOR_SUPPLY("vdd_ddr_rx", NULL),
	REGULATOR_SUPPLY("vddio_vi", NULL),
};

static struct regulator_consumer_supply tps658621_rtc_supply[] = {
	REGULATOR_SUPPLY("vdd_rtc_2", NULL)
};

/* unused */
/*static struct regulator_consumer_supply tps658621_buck_supply[] = {
	REGULATOR_SUPPLY("pll_e", NULL),
};*/

/* Super power voltage rail for the SOC : VDD SOC
*/
static struct regulator_consumer_supply tps658621_soc_supply[] = {
	REGULATOR_SUPPLY("vdd_soc", NULL)
};

/* PLLE voltage rail : AVDD_PLLE -> VDD_1V05
   PEX_CLK voltage rail : AVDD_PLLE -> VDD_1V05
*/
static struct regulator_consumer_supply fixed_buck_tps62290_supply[] = {
	REGULATOR_SUPPLY("avdd_plle", NULL),
};

/* MIPI voltage rail (DSI_CSI): AVDD_DSI_CSI -> VDD_1V2
   Wlan : VCORE_WIFI (VDD_1V2)
*/
static struct regulator_consumer_supply fixed_ldo_tps72012_supply[] = {
	REGULATOR_SUPPLY("avdd_dsi_csi", NULL),
	REGULATOR_SUPPLY("vcore_wifi", NULL)
};

/* PEX_CLK voltage rail : PMU_GPIO-1 -> VDD_1V5
*/
static struct regulator_consumer_supply fixed_ldo_tps74201_supply[] = {
	REGULATOR_SUPPLY("vdd_pex_clk_2", NULL),
};

/* HDMI +5V for the pull-up for DDC : VDDIO_VID
   HDMI +5V for hotplug
   lcd rail (required for crt out) : VDDIO_VGA
*/
static struct regulator_consumer_supply fixed_ldo_tps2051B_supply[] = {
	REGULATOR_SUPPLY("vddio_vid", NULL),
	REGULATOR_SUPPLY("vddio_vga", NULL),
	REGULATOR_SUPPLY("vdd_camera", NULL),
};

/* VAON */
static struct regulator_consumer_supply fixed_vdd_aon_supply[] = { 
	REGULATOR_SUPPLY("vdd_aon", NULL)
};


#define ADJ_REGULATOR_INIT(_id, _minmv, _maxmv, _aon, _bon)	\
	{													\
		.constraints = {								\
			.name = "tps658621_" #_id,					\
			.min_uV = (_minmv)*1000,					\
			.max_uV = (_maxmv)*1000,					\
			.valid_modes_mask = (REGULATOR_MODE_NORMAL |\
					     REGULATOR_MODE_STANDBY),		\
			.valid_ops_mask = (REGULATOR_CHANGE_MODE |	\
					   REGULATOR_CHANGE_STATUS |		\
					   REGULATOR_CHANGE_VOLTAGE),		\
			.always_on	= _aon, 						\
			.boot_on	= _bon, 						\
		},												\
		.num_consumer_supplies = ARRAY_SIZE(tps658621_##_id##_supply),\
		.consumer_supplies = tps658621_##_id##_supply,	\
	}

#define FIXED_REGULATOR_INIT(_id, _mv, _aon, _bon)		\
	{													\
		.constraints = {								\
			.name = #_id,								\
			.min_uV = (_mv)*1000,						\
			.max_uV = (_mv)*1000,						\
			.valid_modes_mask = REGULATOR_MODE_NORMAL,	\
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,	\
			.always_on	= _aon, 						\
			.boot_on	= _bon, 						\
		},												\
		.num_consumer_supplies = ARRAY_SIZE( fixed_##_id##_supply),\
		.consumer_supplies = fixed_##_id##_supply,	\
	}

	
static struct regulator_init_data sm0_data  		 
	= ADJ_REGULATOR_INIT(sm0,  725, 1500, 1, 1); // 1200
static struct regulator_init_data sm1_data  		 
	= ADJ_REGULATOR_INIT(sm1,  725, 1500, 1, 1); // 1000 (min was 1100)
static struct regulator_init_data sm2_data  		 
	= ADJ_REGULATOR_INIT(sm2, 3000, 4550, 1, 1); // 3700
static struct regulator_init_data ldo0_data 		 
	= ADJ_REGULATOR_INIT(ldo0,1250, 3300, 0, 0); // 3300
static struct regulator_init_data ldo1_data 		 
	= ADJ_REGULATOR_INIT(ldo1, 725, 1500, 1, 1); // 1100  V-1V2
static struct regulator_init_data ldo2_data 		 
	= ADJ_REGULATOR_INIT(ldo2, 725, 1500, 1, 1); // 1200  V-RTC
static struct regulator_init_data ldo3_data 		 
	= ADJ_REGULATOR_INIT(ldo3,1250, 3300, 0, 0); // 3300 
static struct regulator_init_data ldo4_data 		 
	= ADJ_REGULATOR_INIT(ldo4,1700, 2000, 1, 1); // 1800
static struct regulator_init_data ldo5_data 		 
	= ADJ_REGULATOR_INIT(ldo5,1250, 3300, 1, 1); // 2850
static struct regulator_init_data ldo6_data 		 
	= ADJ_REGULATOR_INIT(ldo6,1250, 3300, 1, 1); // 2850  V-3V3 USB
static struct regulator_init_data ldo7_data 		 
	= ADJ_REGULATOR_INIT(ldo7,1250, 3300, 0, 0); // 3300  V-SDIO
static struct regulator_init_data ldo8_data 		 
	= ADJ_REGULATOR_INIT(ldo8,1250, 3300, 0, 0); // 1800  V-2V8
static struct regulator_init_data ldo9_data 		 
	= ADJ_REGULATOR_INIT(ldo9,1250, 3300, 1, 1); // 2850
static struct regulator_init_data rtc_data  		 
	= ADJ_REGULATOR_INIT(rtc, 1250, 3350, 1, 1); // 3300
/*static struct regulator_init_data buck_data 
	= ADJ_REGULATOR_INIT(buck,1250, 3350, 0, 0); // 3300*/
	
static struct regulator_init_data soc_data  		 
	= ADJ_REGULATOR_INIT(soc, 1250, 3300, 1, 1);
static struct regulator_init_data ldo_tps74201_data  
	= FIXED_REGULATOR_INIT(ldo_tps74201 , 1500, 0, 0 ); // 1500 (VDD1.5, enabled by PMU_GPIO[0] (0=enabled) - Turn it off as soon as we boot
static struct regulator_init_data buck_tps62290_data 
	= FIXED_REGULATOR_INIT(buck_tps62290, 1050, 1, 1 ); // 1050 (VDD1.05, AVDD_PEX ... enabled by PMU_GPIO[2] (1=enabled)
static struct regulator_init_data ldo_tps72012_data  
	= FIXED_REGULATOR_INIT(ldo_tps72012 , 1200, 0, 0 ); // 1200 (VDD1.2, VCORE_WIFI ...) enabled by PMU_GPIO[1] (1=enabled)

static struct regulator_init_data ldo_tps2051B_data  
	= FIXED_REGULATOR_INIT(ldo_tps2051B , 5000, 1, 1 ); // 5000 (VDDIO_VID), enabled by AP_GPIO Port T, pin2, 
														// (set as input to enable,outpul low to disable). Powers HDMI.
														// Wait 500uS to let it stabilize before returning . Probably also 
														// used for USB host. It should always be kept enabled. Force enabling
														// it at boot.


// vdd_aon is a virtual regulator used to allow frequency and voltage scaling of the CPU/EMC														
static struct regulator_init_data vdd_aon_data =
{
	.constraints = {
		.name = "ldo_vdd_aon",
		.min_uV =  625000,
		.max_uV = 2700000,
		.valid_modes_mask = (REGULATOR_MODE_NORMAL | REGULATOR_MODE_STANDBY),
		.valid_ops_mask = (REGULATOR_CHANGE_MODE |REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE),
		.always_on	= 1,
		.boot_on	= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(fixed_vdd_aon_supply),
	.consumer_supplies = fixed_vdd_aon_supply,
};


#define FIXED_REGULATOR_CONFIG(_id, _mv, _gpio, _activehigh, _itoen, _delay, _atboot, _data)	\
	{												\
		.supply_name 	= #_id,						\
		.microvolts  	= (_mv)*1000,				\
		.gpio        	= _gpio,					\
		.enable_high	= _activehigh,				\
		.startup_delay	= _delay,					\
		.enabled_at_boot= _atboot,					\
		.init_data		= &_data,					\
	}
//		.set_as_input_to_enable = _itoen,			\

/* The next 3 are fixed regulators controlled by PMU GPIOs */
static struct fixed_voltage_config ldo_tps74201_cfg  
	= FIXED_REGULATOR_CONFIG(ldo_tps74201  , 1500, PMU_GPIO0 , 0,0, 200000, 0, ldo_tps74201_data);
static struct fixed_voltage_config buck_tps62290_cfg
	= FIXED_REGULATOR_CONFIG(buck_tps62290 , 1050, PMU_GPIO2 , 1,0, 200000, 1, buck_tps62290_data);
static struct fixed_voltage_config ldo_tps72012_cfg
	= FIXED_REGULATOR_CONFIG(ldo_tps72012  , 1200, PMU_GPIO1 , 1,0, 200000, 1, ldo_tps72012_data);

/* the next one is controlled by a general purpose GPIO */
static struct fixed_voltage_config ldo_tps2051B_cfg
	= FIXED_REGULATOR_CONFIG(ldo_tps2051B  , 5000, ADAM_ENABLE_VDD_VID	, 1,1, 500000, 0, ldo_tps2051B_data);

/* the always on vdd_aon: required for freq. scaling to work */
/*static struct virtual_adj_voltage_config vdd_aon_cfg = {
	.supply_name = "REG-AON",
	.id			 = -1,
	.min_mV 	 =  625,
	.max_mV 	 = 2700,
	.step_mV 	 =   25,
	.mV			 = 1800,
	.init_data	 = &vdd_aon_data,
};*/

#define TPS_ADJ_REG(_id, _data)			\
	{									\
		.id = TPS6586X_ID_##_id,		\
		.name = "tps6586x-regulator",	\
		.platform_data = _data,			\
	}

#define TPS_GPIO_FIX_REG(_id,_data)		\
	{									\
		.id = _id,						\
		.name = "reg-fixed-voltage",	\
		.platform_data = _data,			\
	} 	

/* FIXME: do we have rtc alarm irq? */
static struct tps6586x_rtc_platform_data adam_rtc_data = {
	.irq	= TEGRA_NR_IRQS + TPS6586X_INT_RTC_ALM1,
        .start = {
		.year = 2009,
		.month = 1,
		.day = 1,
        },
	.cl_sel = TPS6586X_RTC_CL_SEL_1_5PF /* use lowest (external 20pF cap) */
};

static struct tps6586x_subdev_info tps_devs[] = {
	TPS_ADJ_REG(SM_0, &sm0_data),
	TPS_ADJ_REG(SM_1, &sm1_data),
	TPS_ADJ_REG(SM_2, &sm2_data),
	TPS_ADJ_REG(LDO_0, &ldo0_data), 
	TPS_ADJ_REG(LDO_1, &ldo1_data),
	TPS_ADJ_REG(LDO_2, &ldo2_data),
	TPS_ADJ_REG(LDO_3, &ldo3_data),
	TPS_ADJ_REG(LDO_4, &ldo4_data),
	TPS_ADJ_REG(LDO_5, &ldo5_data),
	TPS_ADJ_REG(LDO_6, &ldo6_data),
	TPS_ADJ_REG(LDO_7, &ldo7_data),
	TPS_ADJ_REG(LDO_8, &ldo8_data),
	TPS_ADJ_REG(LDO_9, &ldo9_data),
	//TPS_ADJ_REG(LDO_RTC, &rtc_data),
	//TPS_ADJ_REG(LDO_SOC, &soc_data),
	/*TPS_GPIO_FIX_REG(0, &ldo_tps74201_cfg),
	TPS_GPIO_FIX_REG(1, &buck_tps62290_cfg),
	TPS_GPIO_FIX_REG(2, &ldo_tps72012_cfg),*/
	{
		.id		= -1,
		.name		= "tps6586x-rtc",
		.platform_data	= &adam_rtc_data,
	},
};

static struct tps6586x_platform_data tps_platform = {
	.gpio_base = PMU_GPIO_BASE,
	.irq_base  = PMU_IRQ_BASE,
	.subdevs   = tps_devs,
	.num_subdevs = ARRAY_SIZE(tps_devs),	
};

static struct i2c_board_info __initdata adam_regulators[] = {
	{
		I2C_BOARD_INFO("tps6586x", 0x34),
		.platform_data = &tps_platform,
	},
};


#define GPIO_FIXED_REG(_id,_data)		\
	{									\
		.name = "reg-fixed-voltage",	\
		.id = _id,						\
		.dev = { 						\
			.platform_data = & _data,	\
		}, 								\
	} 	

static struct platform_device adam_ldo_tps2051B_reg_device = 
	GPIO_FIXED_REG(3,ldo_tps2051B_cfg); /* id is 3, because 0-2 are already used in the PMU gpio controlled fixed regulators */

/*static struct platform_device adam_vdd_aon_reg_device = 
{
	.name = "reg-virtual-adj-voltage",
	.id = 4,
	.dev = {
		.platform_data = &vdd_aon_cfg,
	},
};*/

/*
static struct regulator_consumer_supply bq24610_consumers[] = {
        {
                .dev = &gpio_vbus.dev,
                .supply = "vbus_draw",
        },
        {
                .dev = &power_supply.dev,
                .supply = "ac_draw",
        },
};


static struct regulator_init_data bq24610_init_data = {
        .constraints = {
                .max_uA         = 500000,
                .valid_ops_mask = REGULATOR_CHANGE_CURRENT,
        },
        .num_consumer_supplies  = ARRAY_SIZE(bq24610_consumers),
        .consumer_supplies      = bq24610_consumers,
};


static struct bq24610_mach_info bq24610_platform_data = {
	.gpio_nce = TEGRA_GPIO_PK7,
	.init_data = &bq24610_init_data,
}

static struct platform_device adam_bq24610_device= {
	.name = "bq24610",
	.dev = {
		.platform_data = &bq24610_platform_data,
	},
}; */

static void reg_off(const char *reg)
{
	int rc;
	struct regulator *regulator;

	regulator = regulator_get(NULL, reg);

	if (IS_ERR(regulator)) {
		pr_err("%s: regulator_get returned %ld\n", __func__,
		       PTR_ERR(regulator));
		return;
	}

	/* force disabling of regulator to turn off system */
	rc = regulator_force_disable(regulator);
	if (rc)
		pr_err("%s: regulator_disable returned %d\n", __func__, rc);
	regulator_put(regulator);
}

static void adam_power_off(void)
{
	/* Power down through NvEC */
	//nvec_poweroff();
	
	/* Then try by powering off supplies */
	reg_off("vdd_sm2");
	reg_off("vdd_core");
	reg_off("vdd_cpu");
	reg_off("vdd_soc");
	local_irq_disable();
	while (1) {
		dsb();
		__asm__ ("wfi");
	}
}

#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,39) || LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
/* missing from defines ... remove ASAP when defined in devices.c */
static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start	= TEGRA_RTC_BASE,
		.end	= TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INT_RTC,
		.end	= INT_RTC,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device tegra_rtc_device = {
	.name		= "tegra_rtc",
	.id		= -1,
	.resource	= tegra_rtc_resources,
	.num_resources	= ARRAY_SIZE(tegra_rtc_resources),
};
#endif

static struct platform_device *adam_power_devices[] __initdata = {
	//&adam_ldo_tps2051B_reg_device,
	//&adam_vdd_aon_reg_device,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)	
	&tegra_pmu_device,
#else
	&pmu_device,
#endif
	//&adam_nvec_mfd,
	&tegra_rtc_device,
	//&adam_bq24610_device,
};

/* Init power management unit of Tegra2 */
int __init adam_power_register_devices(void)
{
	int err;
	//void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
	//u32 pmc_ctrl;

	/* configure the power management controller to trigger PMU
	 * interrupts when low
	 */
	//pmc_ctrl = readl(pmc + PMC_CTRL);
	//writel(pmc_ctrl | PMC_CTRL_INTR_LOW, pmc + PMC_CTRL);

	err = i2c_register_board_info(4, adam_regulators, 1);
	if (err < 0) 
		pr_warning("Unable to initialize regulator\n");

	/* register the poweroff callback */
	pm_power_off = adam_power_off;		

	/* signal that power regulators have fully specified constraints */
	//regulator_has_full_constraints();
	
	/* register all pm devices - This must come AFTER the registration of the TPS i2c interfase,
	   as we need the GPIO definitions exported by that driver */
	//return 0;
	return platform_add_devices(adam_power_devices, ARRAY_SIZE(adam_power_devices));
}

