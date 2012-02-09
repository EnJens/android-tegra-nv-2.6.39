/*
 * arch/arm/mach-tegra/board-adam-gpu.c
 *
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 * Copyright (C) 2011 Jens Andersen <jens.andersen@gmail.com>
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

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <linux/kernel.h>
//#include <mach/tegra_cpufreq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <asm/mach-types.h>
#include <linux/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include "board.h"
#include "devices.h"
#include "gpio-names.h"
#include "board-adam.h"
static bool pqiOn = 0;

static void updatePqiGpio(bool on) {
	if (on && !pqiOn)
		gpio_set_value(ADAM_BL_ENB, 1);
	if (!on || pqiOn) 
		gpio_set_value(ADAM_BL_ENB, 0);
}

static ssize_t store_pqistatus(struct device *dev, struct device_attribute *attr,
                         	const char *buf, size_t count)
{
	int scnCount = 0;
	int scanned = 0;
	scnCount = sscanf(buf, "%d\n", &scanned);
	if (scnCount == 1) {
		pqiOn = !!scanned;
		// Pass the opposite to enable the backlight
		// when PQi mode disabled. Won't matter if it's PQimode is enabled.
		updatePqiGpio(!scanned);
	}
        return count;
}

static ssize_t show_pqistatus(struct device *dev, struct device_attribute *attr,
                        	char *buf)
{        
        return scnprintf(buf, PAGE_SIZE, "%d\n", pqiOn);
}

static DEVICE_ATTR(PQiModeOn, 0666, show_pqistatus, store_pqistatus);


static int adam_backlight_init(struct device *dev)
{
	int ret;

	ret = gpio_request(ADAM_BL_ENB, "backlight_enb");
	if (ret < 0)
		return ret;

	ret = gpio_direction_output(ADAM_BL_ENB, 1);
	if (ret < 0)
		gpio_free(ADAM_BL_ENB);

	gpio_sysfs_set_active_low(ADAM_BL_ENB, 1);

	if (device_create_file(dev, &dev_attr_PQiModeOn))
		pr_err("%s: Failed to create Pqi control!", __func__);
	return ret;
};

static void adam_backlight_exit(struct device *dev)
{
	gpio_set_value(ADAM_BL_ENB, 0);
	gpio_free(ADAM_BL_ENB);
}

static int adam_backlight_notify(struct device *unused, int brightness)
{
	gpio_set_value(ADAM_EN_VDD_PANEL, !!brightness);	
	gpio_set_value(ADAM_LVDS_SHUTDOWN, !!brightness);
	updatePqiGpio(!!brightness);
	return brightness;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
static int adam_disp1_check_fb(struct device *dev, struct fb_info *info);
#endif

static struct platform_pwm_backlight_data adam_backlight_data = {
	.pwm_id		= ADAM_BL_PWM_ID,
	.max_brightness	= 255,
//	.dft_brightness	= 224,
	.dft_brightness	= 200,
	.pwm_period_ns	= 1000000,
	.init		= adam_backlight_init,
	.exit		= adam_backlight_exit,
	.notify		= adam_backlight_notify,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= adam_disp1_check_fb,
#endif
};

static struct platform_device adam_panel_bl_driver = {
	.name = "pwm-backlight",
	.id = -1,
	.dev = {
		.platform_data = &adam_backlight_data,
		},
};

static int adam_panel_enable(void)
{
	gpio_set_value(ADAM_LVDS_SHUTDOWN, 1);
	return 0;
}

static int adam_panel_disable(void)
{
	gpio_set_value(ADAM_LVDS_SHUTDOWN, 0);
	return 0;
}

static struct regulator *adam_hdmi_reg = NULL;
static struct regulator *adam_hdmi_pll = NULL;
static int adam_hdmi_enabled = false;

static int adam_hdmi_enable(void)
{
	if (adam_hdmi_enabled)
		return 0;
		
//	gpio_set_value(ADAM_HDMI_ENB, 1);
	
	adam_hdmi_reg = regulator_get(NULL, "avdd_hdmi");
	if (IS_ERR_OR_NULL(adam_hdmi_reg)) {
//		gpio_set_value(ADAM_HDMI_ENB, 0);
		return PTR_ERR(adam_hdmi_reg);
	}

	adam_hdmi_pll = regulator_get(NULL, "avdd_hdmi_pll");
	if (IS_ERR_OR_NULL(adam_hdmi_pll)) {
		regulator_put(adam_hdmi_reg);
		adam_hdmi_reg = NULL;
//		gpio_set_value(ADAM_HDMI_ENB, 0);
		return PTR_ERR(adam_hdmi_pll);
	}
	
	regulator_enable(adam_hdmi_reg);
	regulator_enable(adam_hdmi_pll);
	adam_hdmi_enabled = true;
	return 0;
}

static int adam_hdmi_disable(void)
{
	if (!adam_hdmi_enabled)
		return 0;
		
//	gpio_set_value(ADAM_HDMI_ENB, 0);
	
	regulator_disable(adam_hdmi_reg);
	regulator_disable(adam_hdmi_pll);
	regulator_put(adam_hdmi_reg);
	adam_hdmi_reg = NULL;
	regulator_put(adam_hdmi_pll);
	adam_hdmi_pll = NULL;
	adam_hdmi_enabled = false;
	return 0;
}

#define TEGRA_ROUND_ALLOC(x) (((x) + 4095) & ((unsigned)(-4096)))

/* If using 1024x600 panel (Shuttle default panel) */

/* Frame buffer size assuming 16bpp color */
#define ADAM_FB_SIZE TEGRA_ROUND_ALLOC(1024*600*(32/8)*ADAM_FB_PAGES)

static struct tegra_dc_mode adam_panel_modes[] = {
	{
		.pclk = 42430000,
		.h_ref_to_sync = 4,
		.v_ref_to_sync = 2,
		.h_sync_width = 136,
		.v_sync_width = 4,
		.h_back_porch = 138,
		.v_back_porch = 21,
		.h_active = 1024,
		.v_active = 600,
		.h_front_porch = 34,
		.v_front_porch = 4,
	},
};

static struct tegra_fb_data adam_fb_data = {
	.win		= 0,
	.xres		= 1024,
	.yres		= 600,
	.bits_per_pixel	= 32,
};

#if defined(ADAM_1920x1080HDMI)

/* Frame buffer size assuming 16bpp color and 2 pages for page flipping */
#define ADAM_FB_HDMI_SIZE TEGRA_ROUND_ALLOC(1920*1080*(16/8)*2)

static struct tegra_fb_data adam_hdmi_fb_data = {
	.win		= 0,
	.xres		= 1920,
	.yres		= 1080,
	.bits_per_pixel	= 16,
};

#else

#define ADAM_FB_HDMI_SIZE TEGRA_ROUND_ALLOC(1280*720*(16/8)*2)

static struct tegra_fb_data adam_hdmi_fb_data = {
	.win		= 0,
	.xres		= 1280,
	.yres		= 720,
	.bits_per_pixel	= 16,
};
#endif

static struct tegra_dc_out adam_disp1_out = {
	.type		= TEGRA_DC_OUT_RGB,

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

        /* Enable dithering. Tegra also supports error
               diffusion, but when the active region is less
               than 640 pixels wide. */
       .dither         = TEGRA_DC_ERRDIFF_DITHER,

	.height 	= 136, /* mm */
	.width 		= 217, /* mm */
	
	.modes	 	= adam_panel_modes,
	.n_modes 	= ARRAY_SIZE(adam_panel_modes),

	.enable		= adam_panel_enable,
	.disable	= adam_panel_disable,
};

static struct tegra_dc_out adam_hdmi_out = {
	.type		= TEGRA_DC_OUT_HDMI,
	.flags		= TEGRA_DC_OUT_HOTPLUG_HIGH,

	.dcc_bus	= 1,
	.hotplug_gpio	= ADAM_HDMI_HPD,

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.enable		= adam_hdmi_enable,
	.disable	= adam_hdmi_disable,
};

static struct tegra_dc_platform_data adam_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
//	.emc_clk_rate	= 300000000,
	.default_out	= &adam_disp1_out,
	.fb		= &adam_fb_data,
};

static struct tegra_dc_platform_data adam_hdmi_pdata = {
	.flags		= 0,
	.default_out	= &adam_hdmi_out,
	.fb		= &adam_hdmi_fb_data,
};

/* Estimate memory layout for GPU */
#define ADAM_GPU_MEM_START	(ADAM_MEM_SIZE - ADAM_GPU_MEM_SIZE)
#define ADAM_FB_BASE		(ADAM_GPU_MEM_START)
#define ADAM_FB_HDMI_BASE 	(ADAM_GPU_MEM_START + ADAM_FB_SIZE)
#define ADAM_CARVEOUT_BASE 	(ADAM_GPU_MEM_START + ADAM_FB_SIZE + ADAM_FB_HDMI_SIZE)
#define ADAM_CARVEOUT_SIZE	(ADAM_MEM_SIZE - ADAM_CARVEOUT_BASE)

/* Display Controller */
static struct resource adam_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= ADAM_FB_BASE,
		.end	= ADAM_FB_BASE + ADAM_FB_SIZE - 1, 
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource adam_disp2_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_B_GENERAL,
		.end	= INT_DISPLAY_B_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY2_BASE,
		.end	= TEGRA_DISPLAY2_BASE + TEGRA_DISPLAY2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= ADAM_FB_HDMI_BASE,
		.end	= ADAM_FB_HDMI_BASE + ADAM_FB_HDMI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "hdmi_regs",
		.start	= TEGRA_HDMI_BASE,
		.end	= TEGRA_HDMI_BASE + TEGRA_HDMI_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct nvhost_device adam_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= adam_disp1_resources,
	.num_resources	= ARRAY_SIZE(adam_disp1_resources),
	.dev = {
		.platform_data = &adam_disp1_pdata,
	},
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
static int adam_disp1_check_fb(struct device *dev, struct fb_info *info)
{
	return info->device == &adam_disp1_device.dev;
}
#endif

static struct nvhost_device adam_disp2_device = {
	.name		= "tegradc",
	.id		= 1,
	.resource	= adam_disp2_resources,
	.num_resources	= ARRAY_SIZE(adam_disp2_resources),
	.dev = {
		.platform_data = &adam_hdmi_pdata,
	},
};

static struct nvmap_platform_carveout adam_carveouts[] = {
	[0] =  NVMAP_HEAP_CARVEOUT_IRAM_INIT,
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= ADAM_CARVEOUT_BASE,
		.size		= ADAM_CARVEOUT_SIZE,
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data adam_nvmap_data = {
	.carveouts	= adam_carveouts,
	.nr_carveouts	= ARRAY_SIZE(adam_carveouts),
};

static struct platform_device adam_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &adam_nvmap_data,
	},
};

static struct platform_device *adam_gfx_devices[] __initdata = {
	&adam_nvmap_device,
	&tegra_grhost_device,
	&tegra_pwfm0_device,
	&adam_panel_bl_driver,
	&tegra_gart_device,
	&tegra_avp_device,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
/* put early_suspend/late_resume handlers here for the display in order
 * to keep the code out of the display driver, keeping it closer to upstream
 */
struct early_suspend adam_panel_early_suspender;

static void adam_panel_early_suspend(struct early_suspend *h)
{
        unsigned i;

        /* power down LCD, add use a black screen for HDMI */
        if (num_registered_fb > 0)
                fb_blank(registered_fb[0], FB_BLANK_POWERDOWN);
        if (num_registered_fb > 1)
                fb_blank(registered_fb[1], FB_BLANK_NORMAL);
#ifdef CONFIG_TEGRA_CONVSERVATIVE_GOV_ON_EARLYSUPSEND
        cpufreq_save_default_governor();
        cpufreq_set_conservative_governor();
        cpufreq_set_conservative_governor_param(
                SET_CONSERVATIVE_GOVERNOR_UP_THRESHOLD,
                SET_CONSERVATIVE_GOVERNOR_DOWN_THRESHOLD);
#endif
}

static void adam_panel_late_resume(struct early_suspend *h)
{
        unsigned i;
#ifdef CONFIG_TEGRA_CONVSERVATIVE_GOV_ON_EARLYSUPSEND
        cpufreq_restore_default_governor();
#endif
        for (i = 0; i < num_registered_fb; i++)
                fb_blank(registered_fb[i], FB_BLANK_UNBLANK);
}
#endif 

int __init adam_gpu_register_devices(void)
{
	struct resource *res;
	int err;
	
#if defined(DYNAMIC_GPU_MEM)
	/* Plug in framebuffer 1 memory area and size */
	if (tegra_fb_start > 0 && tegra_fb_size > 0) {
		res = nvhost_get_resource_byname(&adam_disp1_device,
			IORESOURCE_MEM, "fbmem");
		res->start = tegra_fb_start;
		res->end = tegra_fb_start + tegra_fb_size - 1;
	}

	/* Plug in framebuffer 2 memory area and size */
	if (tegra_fb2_start > 0 && tegra_fb2_size > 0) {
		res = nvhost_get_resource_byname(&adam_disp2_device,
			IORESOURCE_MEM, "fbmem");
			res->start = tegra_fb2_start;
			res->end = tegra_fb2_start + tegra_fb2_size - 1;
	}
	
	/* Plug in carveout memory area and size */
	if (tegra_carveout_start > 0 && tegra_carveout_size > 0) {
		adam_carveouts[1].base = tegra_carveout_start;
		adam_carveouts[1].size = tegra_carveout_size;
	}
#endif

	gpio_request(ADAM_EN_VDD_PANEL, "en_vdd_pnl");
	gpio_direction_output(ADAM_EN_VDD_PANEL, 1);
	
	gpio_request(ADAM_BL_VDD, "bl_vdd");
	gpio_direction_output(ADAM_BL_VDD, 1);
	
//	gpio_request(ADAM_HDMI_ENB, "hdmi_5v_en");
//	gpio_direction_output(ADAM_HDMI_ENB, 1);
	
	gpio_request(ADAM_LVDS_SHUTDOWN, "lvds_shdn");
	gpio_direction_output(ADAM_LVDS_SHUTDOWN, 1);
	

#ifdef CONFIG_HAS_EARLYSUSPEND
        adam_panel_early_suspender.suspend = adam_panel_early_suspend;
        adam_panel_early_suspender.resume = adam_panel_late_resume;
        adam_panel_early_suspender.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
        register_early_suspend(&adam_panel_early_suspender);
#endif

	err = platform_add_devices(adam_gfx_devices,
				   ARRAY_SIZE(adam_gfx_devices));
				   
#if defined(DYNAMIC_GPU_MEM)				   
	/* Move the bootloader framebuffer to our framebuffer */
	if (tegra_bootloader_fb_start > 0 && tegra_fb_start > 0 &&
		tegra_fb_size > 0 && tegra_bootloader_fb_size > 0) {
		tegra_move_framebuffer(tegra_fb_start, tegra_bootloader_fb_start,
			min(tegra_fb_size, tegra_bootloader_fb_size)); 		
	}		
#endif

	/* Register the framebuffers */
	if (!err)
		err = nvhost_device_register(&adam_disp1_device);

	if (!err)
		err = nvhost_device_register(&adam_disp2_device);

	return err;
}

#if defined(DYNAMIC_GPU_MEM)
int __init adam_protected_aperture_init(void)
{
	if (tegra_grhost_aperture > 0) {
		tegra_protected_aperture_init(tegra_grhost_aperture);
	}
	return 0;
}
late_initcall(adam_protected_aperture_init);
#endif

