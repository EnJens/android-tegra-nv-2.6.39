/*
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com> 
	 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
	 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/adt7461.h>

#include "board-adam.h"
#include "gpio-names.h"
#include "cpu-tegra.h"

static struct i2c_board_info __initdata adam_i2c_bus0_sensor_info[] = {
	{
		I2C_BOARD_INFO("bq20z75-battery", 0x0B),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PH2),
	},
	{
		I2C_BOARD_INFO("so340010_kbd", 0x2c),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PV6),
	},
};

static struct i2c_board_info __initdata adam_i2c_bus2_sensor_info[] = {
	{
		I2C_BOARD_INFO("isl29023", 0x44),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PV5),
	},
	{
		I2C_BOARD_INFO("lis3lv02d", 0x1C),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PJ0),
	},
	{
		I2C_BOARD_INFO("mmc31xx", 0x30),
	},
};

static struct adt7461_platform_data adam_adt7461_pdata = {
	.supported_hwrev = true,
	.ext_range = false,
	.therm2 = true,
	.conv_rate = 0x05,
	.offset = 0,
	.hysteresis = 0,
	.shutdown_ext_limit = 115,
	.shutdown_local_limit = 120,
	.throttling_ext_limit = 90,
	.alarm_fn = tegra_throttling_enable,
};


static struct i2c_board_info __initdata adam_i2c_bus4_sensor_info[] = {
	{
		I2C_BOARD_INFO("adt7461", 0x4C),
		.irq = TEGRA_GPIO_TO_IRQ(ADAM_TEMP_ALERT),
		.platform_data = &adam_adt7461_pdata,
	},
};

int __init adam_sensors_register_devices(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PV5);
	gpio_request(TEGRA_GPIO_PV5, "isl29023_irq");
	gpio_direction_input(TEGRA_GPIO_PV5);

	tegra_gpio_enable(TEGRA_GPIO_PH2);
	gpio_request(TEGRA_GPIO_PH2, "ac_present_irq");
	gpio_direction_input(TEGRA_GPIO_PH2);

	tegra_gpio_enable(TEGRA_GPIO_PJ0);
	gpio_request(TEGRA_GPIO_PJ0, "lis33de_irq");
	gpio_direction_input(TEGRA_GPIO_PJ0);

	tegra_gpio_enable(TEGRA_GPIO_PV6);
	gpio_request(TEGRA_GPIO_PV6, "so340010_kbd_irq");
	gpio_direction_input(TEGRA_GPIO_PV6);
	
		tegra_gpio_enable(ADAM_TEMP_ALERT);
	gpio_request(ADAM_TEMP_ALERT, "adt7461_temp_alert_irq");
	gpio_direction_input(ADAM_TEMP_ALERT);

	i2c_register_board_info(0, adam_i2c_bus0_sensor_info,
	                        ARRAY_SIZE(adam_i2c_bus0_sensor_info));
	i2c_register_board_info(4, adam_i2c_bus4_sensor_info,
	                        ARRAY_SIZE(adam_i2c_bus4_sensor_info));
	return i2c_register_board_info(2, adam_i2c_bus2_sensor_info,
	                               ARRAY_SIZE(adam_i2c_bus2_sensor_info));
}
