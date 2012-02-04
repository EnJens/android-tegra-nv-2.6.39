/*
 * arch/arm/mach-tegra/board-adam.c
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
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
#include <linux/mfd/tps6586x.h>
#include "fuse.h"



#include <mach/io.h>
#include <mach/w1.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/nand.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>
#include <mach/gpio.h>
#include <mach/clk.h>
#include <mach/usb_phy.h>
//#include <mach/tegra2_i2s.h>
#include <mach/system.h>
#include <mach/nvmap.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,38)	
#include <mach/suspend.h>
#else
#include "pm.h"
#endif

#include <linux/usb/f_accessory.h>

#include "board.h"
#include "board-adam.h"
#include "clock.h"
#include "gpio-names.h"
#include "devices.h"
#include "wakeups-t2.h"


#define PMC_CTRL                0x0
#define PMC_CTRL_INTR_LOW       (1 << 17)

/* NVidia bootloader tags */
#define ATAG_NVIDIA			0x41000801
#define MAX_MEMHDL			8

struct tag_tegra {
        __u32 bootarg_len;
        __u32 bootarg_key;
        __u32 bootarg_nvkey;
        __u32 bootarg[];
};

struct memhdl {
        __u32 id;
        __u32 start;
        __u32 size;
};

enum {
        RM = 1,
        DISPLAY,
        FRAMEBUFFER,
        CHIPSHMOO,
        CHIPSHMOO_PHYS,
        CARVEOUT,
        WARMBOOT,
};

static int num_memhdl = 0;

static struct memhdl nv_memhdl[MAX_MEMHDL];

static const char atag_ids[][16] = {
        "RM             ",
        "DISPLAY        ",
        "FRAMEBUFFER    ",
        "CHIPSHMOO      ",
        "CHIPSHMOO_PHYS ",
        "CARVEOUT       ",
        "WARMBOOT       ",
};

static int __init parse_tag_nvidia(const struct tag *tag)
{
        int i;
        struct tag_tegra *nvtag = (struct tag_tegra *)tag;
        __u32 id;

        switch (nvtag->bootarg_nvkey) {
        case FRAMEBUFFER:
                id = nvtag->bootarg[1];
                for (i=0; i<num_memhdl; i++)
             		if (nv_memhdl[i].id == id) {
	    	 		tegra_bootloader_fb_start = nv_memhdl[i].start;
		        	tegra_bootloader_fb_size = nv_memhdl[i].size;

     		 	}
                break;
        case WARMBOOT:
                id = nvtag->bootarg[1];
                for (i=0; i<num_memhdl; i++) {
                        if (nv_memhdl[i].id == id) {
                                tegra_lp0_vec_start = nv_memhdl[i].start;
                                tegra_lp0_vec_size = nv_memhdl[i].size;
                        }
                }
                break;
        }

        if (nvtag->bootarg_nvkey & 0x10000) {
                char pmh[] = " PreMemHdl     ";
                id = nvtag->bootarg_nvkey;
                if (num_memhdl < MAX_MEMHDL) {
                        nv_memhdl[num_memhdl].id = id;
                        nv_memhdl[num_memhdl].start = nvtag->bootarg[1];
                        nv_memhdl[num_memhdl].size = nvtag->bootarg[2];
                        num_memhdl++;
                }
                pmh[11] = '0' + id;
                print_hex_dump(KERN_INFO, pmh, DUMP_PREFIX_NONE,
                                32, 4, &nvtag->bootarg[0], 4*(tag->hdr.size-2), false);
        }
        else if (nvtag->bootarg_nvkey <= ARRAY_SIZE(atag_ids))
                print_hex_dump(KERN_INFO, atag_ids[nvtag->bootarg_nvkey-1], DUMP_PREFIX_NONE,
                                32, 4, &nvtag->bootarg[0], 4*(tag->hdr.size-2), false);
        else
                pr_warning("unknown ATAG key %d\n", nvtag->bootarg_nvkey);

        return 0;
}
__tagtable(ATAG_NVIDIA, parse_tag_nvidia);



static unsigned long ramconsole_start = SZ_512M*2 - SZ_2M;
static unsigned long ramconsole_size = SZ_1M;

static struct resource ram_console_resources[] = {
	{
		/* .start and .end filled in later */
		.flags  = IORESOURCE_MEM,
	},
};

//static struct ram_console_platform_data ram_console_pdata;

static struct platform_device ram_console_device = {
	.name           = "ram_console",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resources),
	.resource       = ram_console_resources,
	//.dev    = {
	//	.platform_data = &ram_console_pdata,
	//},
};

static int __init stingray_ramconsole_arg(char *options)
{
	char *p = options;

	ramconsole_size = memparse(p, &p);
	if (*p == '@')
		ramconsole_start = memparse(p+1, &p);

	return 0;
}
early_param("ramconsole", stingray_ramconsole_arg);

static atomic_t adam_gps_mag_powered = ATOMIC_INIT(0);
void adam_gps_mag_poweron(void)
{
	if (atomic_inc_return(&adam_gps_mag_powered) == 1) {
		pr_info("Enabling GPS/Magnetic module\n");
		/* 3G/GPS power on sequence */
		gpio_set_value(ADAM_GPSMAG_DISABLE, 1); /* Enable power */
		msleep(2);
	}
}
EXPORT_SYMBOL_GPL(adam_gps_mag_poweron);

void adam_gps_mag_poweroff(void)
{
	if (atomic_dec_return(&adam_gps_mag_powered) == 0) {
		pr_info("Disabling GPS/Magnetic module\n");
		/* 3G/GPS power on sequence */
		gpio_set_value(ADAM_GPSMAG_DISABLE, 0); /* Disable power */
		msleep(2);
	}
}
EXPORT_SYMBOL_GPL(adam_gps_mag_poweroff);

static atomic_t adam_gps_mag_inited = ATOMIC_INIT(0);
void adam_gps_mag_init(void)
{
	if (atomic_inc_return(&adam_gps_mag_inited) == 1) {
		gpio_request(ADAM_GPSMAG_DISABLE, "gps_disable");
		gpio_direction_output(ADAM_GPSMAG_DISABLE, 0);
	}
}
EXPORT_SYMBOL_GPL(adam_gps_mag_init);

void adam_gps_mag_deinit(void)
{
	atomic_dec(&adam_gps_mag_inited);
}
EXPORT_SYMBOL_GPL(adam_gps_mag_deinit);

static struct clk *wifi_32k_clk;
int adam_bt_wifi_gpio_init(void)
{
	static bool inited = 0;
	// Check to see if we've already been init'ed.
	if (inited) 
		return 0;
	wifi_32k_clk = clk_get_sys(NULL, "blink");
        if (IS_ERR(wifi_32k_clk)) {
                pr_err("%s: unable to get blink clock\n", __func__);
                return -1;
        }
	gpio_request(ADAM_WLAN_POWER, "bt_wifi_power");
        tegra_gpio_enable(ADAM_WLAN_POWER);
	gpio_direction_output(ADAM_WLAN_POWER, 0);
	inited = 1;
	return 0;	
}
EXPORT_SYMBOL_GPL(adam_bt_wifi_gpio_init);

int adam_bt_wifi_gpio_set(bool on)
{
       static int count = 0;
	if (IS_ERR(wifi_32k_clk)) {
		pr_err("%s: Clock wasn't obtained\n", __func__);
		return -1;
	}
				
	if (on) {
		if (count == 0) {
			gpio_set_value(ADAM_WLAN_POWER, 1);
        		mdelay(100);
			clk_enable(wifi_32k_clk);
		}
		count++;
	} else {
		if (count == 0) {
			pr_err("%s: Unbalanced wifi/bt power disable requests\n", __func__);
			return -1;
		} else if (count == 1) {
			        gpio_set_value(ADAM_WLAN_POWER, 0);
        			mdelay(100);
				clk_disable(wifi_32k_clk);
		} 
		--count;
	}
	return 0;		
}
EXPORT_SYMBOL_GPL(adam_bt_wifi_gpio_set);

static struct tegra_utmip_config utmi_phy_config[] = {
	[0] = {
		.hssync_start_delay = 0,
		.idle_wait_delay 	= 17,
		.elastic_limit 		= 16,
		.term_range_adj 	= 6, 	/*  xcvr_setup = 9 with term_range_adj = 6 gives the maximum guard around */
		.xcvr_setup 		= 9, 	/*  the USB electrical spec. This is true across fast and slow chips, high */
									/*  and low voltage and hot and cold temperatures */
		.xcvr_lsfslew 		= 2,	/*  -> To slow rise and fall times in low speed eye diagrams in host mode */
		.xcvr_lsrslew 		= 2,	/*                                                                        */
	},
	[1] = {
		.hssync_start_delay = 0,
		.idle_wait_delay 	= 17,
		.elastic_limit 		= 16,
		.term_range_adj 	= 6,	/*  -> xcvr_setup = 9 with term_range_adj = 6 gives the maximum guard around */
		.xcvr_setup 		= 9,	/*     the USB electrical spec. This is true across fast and slow chips, high */
									/*     and low voltage and hot and cold temperatures */
		.xcvr_lsfslew 		= 2,	/*  -> To slow rise and fall times in low speed eye diagrams in host mode */
		.xcvr_lsrslew 		= 2,	/*                                                                        */
	},
};

/* ULPI is managed by an SMSC3317 on the Harmony board */
static struct tegra_ulpi_config ulpi_phy_config = {
	// Is this even right?
	.reset_gpio = TEGRA_GPIO_PG2,
	.clk = "cdev2",
};

static struct tegra_ulpi_config adam_ehci2_ulpi_phy_config = {
	.reset_gpio = ADAM_USB1_RESET,
	.clk = "cdev2",
};

static struct tegra_ehci_platform_data adam_ehci2_ulpi_platform_data = {
	.operating_mode = TEGRA_USB_HOST,
	.power_down_on_bus_suspend = 1,
	.phy_config = &adam_ehci2_ulpi_phy_config,
	.phy_type = TEGRA_USB_PHY_TYPE_LINK_ULPI,
};

static struct usb_phy_plat_data tegra_usb_phy_pdata[] = {
	[0] = {
			.instance = 0,
			//.vbus_irq = TPS6586X_INT_BASE + TPS6586X_INT_USB_DET,
			.vbus_gpio = ADAM_USB0_VBUS,
	},
	[1] = {
			.instance = 1,
			.vbus_gpio = -1,
	},
	[2] = {
			.instance = 2,
			.vbus_gpio = -1,
	},
};

static struct tegra_ehci_platform_data tegra_ehci_pdata[] = {
	[0] = {
			.phy_config = &utmi_phy_config[0],
			.operating_mode = TEGRA_USB_OTG,
			.power_down_on_bus_suspend = 0,
	},
	[1] = {
			.phy_config = &ulpi_phy_config,
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
			.phy_type = TEGRA_USB_PHY_TYPE_LINK_ULPI,
	},
	[2] = {
			.phy_config = &utmi_phy_config[1],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
	},
};

static struct tegra_otg_platform_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci_pdata[0],
};

static void adam_board_suspend(int lp_state, enum suspend_stage stg)
{
        if ((lp_state == TEGRA_SUSPEND_LP1) && (stg == TEGRA_SUSPEND_BEFORE_CPU))
                tegra_console_uart_suspend();
}

static void adam_board_resume(int lp_state, enum resume_stage stg)
{
        if ((lp_state == TEGRA_SUSPEND_LP1) && (stg == TEGRA_RESUME_AFTER_CPU))
                tegra_console_uart_resume();
}


static struct tegra_suspend_platform_data adam_suspend = {
	.cpu_timer = 2000,
	.cpu_off_timer = 100,
	.core_timer = 0x7e7e,
	.core_off_timer = 0xf,
        .corereq_high = false,
	.sysclkreq_high = true,
	.suspend_mode = TEGRA_SUSPEND_LP0,
        .board_suspend = adam_board_suspend,
        .board_resume = adam_board_resume,

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,38) /* NB: 2.6.39+ handles this automatically */
	.separate_req = true,	
	.wake_enb = ADAM_WAKE_KEY_POWER | 
				ADAM_WAKE_KEY_RESUME | 
				TEGRA_WAKE_RTC_ALARM,
	.wake_high = TEGRA_WAKE_RTC_ALARM,
	.wake_low = ADAM_WAKE_KEY_POWER | 
				ADAM_WAKE_KEY_RESUME,
	.wake_any = 0,
#endif
};


static void adam_usb_init(void)
{
	tegra_usb_phy_init(tegra_usb_phy_pdata, ARRAY_SIZE(tegra_usb_phy_pdata));
	/* OTG should be the first to be registered */
	//gpio_request(ADAM_USB0_VBUS, "USB0 VBUS");
	//gpio_direction_output(ADAM_USB0_VBUS, 0 );

	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	platform_device_register(&tegra_udc_device);
//	platform_device_register(&tegra_ehci1_device);

	tegra_ehci3_device.dev.platform_data = &tegra_ehci_pdata[2];
	platform_device_register(&tegra_ehci3_device);

}


static void __init tegra_adam_init(void)
{
	struct clk *clk;
	struct resource *res;
	       void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
        void __iomem *chip_id = IO_ADDRESS(TEGRA_APB_MISC_BASE) + 0x804;
        u32 pmc_ctrl;
        u32 minor;

        minor = (readl(chip_id) >> 16) & 0xf;
        /* A03 (but not A03p) chips do not support LP0 */
        if (minor == 3 && !(tegra_spare_fuse(18) || tegra_spare_fuse(19)))
                adam_suspend.suspend_mode = TEGRA_SUSPEND_LP1;
	printk("Debug: Suspend Data, %x, %d, %d", minor, !!tegra_spare_fuse(18),
							!!tegra_spare_fuse(19));

        /* configure the power management controller to trigger PMU
         * interrupts when low */
        pmc_ctrl = readl(pmc + PMC_CTRL);
        writel(pmc_ctrl | PMC_CTRL_INTR_LOW, pmc + PMC_CTRL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)	
	tegra_common_init();
#endif

	/* force consoles to stay enabled across suspend/resume */
	// console_suspend_enabled = 0;	

	/* Init the suspend information */
	
	tegra_init_suspend(&adam_suspend);

	/* Set the SDMMC1 (wifi) tap delay to 6.  This value is determined
	 * based on propagation delay on the PCB traces. */
	clk = clk_get_sys("sdhci-tegra.0", NULL);
	if (!IS_ERR(clk)) {
		tegra_sdmmc_tap_delay(clk, 6);
		clk_put(clk);
	} else {
		pr_err("Failed to set wifi sdmmc tap delay\n");
	}

	res = platform_get_resource(&ram_console_device, IORESOURCE_MEM, 0);
	res->start = ramconsole_start;
	res->end = ramconsole_start + ramconsole_size - 1;

	platform_device_register(&ram_console_device);

	/* Initialize the pinmux */
	adam_pinmux_init();

	/* Initialize the clocks - clocks require the pinmux to be initialized first */
	adam_clks_init();

	/* Register i2c devices - required for Power management and MUST be done before the power register */
	adam_i2c_register_devices();

	/* Register the power subsystem - Including the poweroff handler - Required by all the others */
	adam_power_register_devices();
	
	/* Register the USB device */
	//adam_usb_register_devices();
	adam_usb_init();

	/* Register UART devices */
	adam_uart_register_devices();
	
	/* Register SPI devices */
	adam_spi_register_devices();

	/* Register GPU devices */
	adam_gpu_register_devices();

	/* Register Audio devices */
	adam_audio_register_devices();

	/* Register AES encryption devices */
	adam_aes_register_devices();

	/* Register Watchdog devices */
	adam_wdt_register_devices();

	/* Register all the keyboard devices */
	adam_keyboard_register_devices();
	
	/* Register touchscreen devices */
	adam_touch_register_devices();
	
	/* Register accelerometer device */
	adam_sensors_register_devices();
	
	/* Register wlan powermanagement devices */
//	adam_wlan_pm_register_devices();
	
	/* Register gps powermanagement devices */
	adam_gps_pm_register_devices();

	/* Register gsm powermanagement devices */
	adam_gsm_pm_register_devices();
	
	/* Register Bluetooth powermanagement devices */
	//adam_bt_pm_register_devices();
	adam_bt_rfkill();
	adam_setup_bluesleep();

	/* Register Camera powermanagement devices */
//	adam_camera_register_devices();

	/* Register NAND flash devices */
	adam_nand_register_devices();

	/* Register SDHCI devices */
	adam_sdhci_register_devices();
	
	adam_gps_mag_init();
	adam_gps_mag_poweron();
	tegra_release_bootloader_fb();
#if 0
	/* Finally, init the external memory controller and memory frequency scaling
   	   NB: This is not working on ADAM. And seems there is no point in fixing it,
	   as the EMC clock is forced to the maximum speed as soon as the 2D/3D engine
	   starts.*/
	adam_init_emc();
#endif
	
}

static void __init tegra_adam_reserve(void)
{
	long ret = 0;
	if (memblock_reserve(0x0, 4096) < 0)
		pr_warn("Cannot reserve first 4K of memory for safety\n");

	ret = memblock_remove(SZ_512M*2 - SZ_2M, SZ_2M);
	if (ret)
		pr_info("Failed to remove ram console\n");
	else
		pr_info("Reserved %08lx@%08lx for ram console\n",
			ramconsole_start, ramconsole_size);

#if defined(DYNAMIC_GPU_MEM)
	/* Reserve the graphics memory */
	tegra_reserve(ADAM_GPU_MEM_SIZE, ADAM_FB1_MEM_SIZE, ADAM_FB2_MEM_SIZE);
#endif
}

static void __init tegra_adam_fixup(struct machine_desc *desc,
	struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = ADAM_MEM_BANKS;
	mi->bank[0].start = PHYS_OFFSET;
#if defined(DYNAMIC_GPU_MEM)
	mi->bank[0].size  = ADAM_MEM_SIZE;
#else
	mi->bank[0].size  = ADAM_MEM_SIZE - ADAM_GPU_MEM_SIZE;
#endif
	// Adam has two 512MB banks. Easier to hardcode if we leave ADAM_MEM_SIZE at 512MB
	mi->bank[1].start = ADAM_MEM_SIZE;
	mi->bank[1].size = ADAM_MEM_SIZE;
} 

MACHINE_START(HARMONY, "harmony")
	.boot_params	= 0x00000100,
	.map_io         = tegra_map_common_io,
	.init_early     = tegra_init_early,
	.init_irq       = tegra_init_irq,
	.timer          = &tegra_timer,
	.init_machine	= tegra_adam_init,
	.reserve		= tegra_adam_reserve,
	.fixup			= tegra_adam_fixup,
MACHINE_END

#if 0
#define PMC_WAKE_STATUS 0x14

static int adam_wakeup_key(void)
{
	unsigned long status = 
		readl(IO_ADDRESS(TEGRA_PMC_BASE) + PMC_WAKE_STATUS);
	return status & TEGRA_WAKE_GPIO_PV2 ? KEY_POWER : KEY_RESERVED;
}
#endif


