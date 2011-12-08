/*
 * A iio driver for the light sensor ISL 29023.
 *
 * Hwmon driver for monitoring ambient light intensity in luxi, proximity
 * sensing and infrared sensing.
 *
 * Copyright (c) 2010, NVIDIA Corporation.
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
 * 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.
 */

#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "../iio.h"

#define CONVERSION_TIME_MS		100

#define ISL29023_REG_ADD_COMMAND1	0x00
#define COMMMAND1_OPMODE_SHIFT		5
#define COMMMAND1_OPMODE_MASK		(7 << COMMMAND1_OPMODE_SHIFT)
#define COMMMAND1_OPMODE_POWER_DOWN	0
#define COMMMAND1_OPMODE_ALS_ONCE	1
#define COMMMAND1_OPMODE_IR_ONCE	2
#define COMMMAND1_OPMODE_PROX_ONCE	3

#define ISL29023_REG_ADD_COMMANDII	0x01
#define COMMANDII_RESOLUTION_SHIFT	2
#define COMMANDII_RESOLUTION_MASK	(0x3 << COMMANDII_RESOLUTION_SHIFT)

#define COMMANDII_RANGE_SHIFT		0
#define COMMANDII_RANGE_MASK		(0x3 << COMMANDII_RANGE_SHIFT)

#define COMMANDII_SCHEME_SHIFT		7
#define COMMANDII_SCHEME_MASK		(0x1 << COMMANDII_SCHEME_SHIFT)

#define ISL29023_REG_ADD_DATA_LSB	0x02
#define ISL29023_REG_ADD_DATA_MSB	0x03
#define ISL29023_MAX_REGS		ISL29023_REG_ADD_DATA_MSB

struct isl29023_chip {
	struct iio_dev		*indio_dev;
	struct i2c_client	*client;
	struct mutex		lock;
	unsigned int		range;
	unsigned int		adc_bit;
	int			prox_scheme;
	u8			reg_cache[ISL29023_MAX_REGS];
	struct regulator 	*regulator;
};

static bool isl29023_write_data(struct i2c_client *client, u8 reg,
	u8 val, u8 mask, u8 shift)
{
	u8 regval;
	int ret = 0;
	struct isl29023_chip *chip = i2c_get_clientdata(client);

	regval = chip->reg_cache[reg];
	regval &= ~mask;
	regval |= val << shift;

	ret = i2c_smbus_write_byte_data(client, reg, regval);
	if (ret) {
		dev_err(&client->dev, "Write to device fails status %x\n", ret);
		return false;
	}
	chip->reg_cache[reg] = regval;
	return true;
}

static bool isl29023_set_range(struct i2c_client *client, unsigned long range,
		unsigned int *new_range)
{
	unsigned long supp_ranges[] = {1000, 4000, 16000, 64000};
	int i;

	for (i = 0; i < (ARRAY_SIZE(supp_ranges) -1); ++i) {
		if (range <= supp_ranges[i])
			break;
	}
	*new_range = (unsigned int)supp_ranges[i];

	return isl29023_write_data(client, ISL29023_REG_ADD_COMMANDII,
		i, COMMANDII_RANGE_MASK, COMMANDII_RANGE_SHIFT);
}

static bool isl29023_set_resolution(struct i2c_client *client,
			unsigned long adcbit, unsigned int *conf_adc_bit)
{
	unsigned long supp_adcbit[] = {16, 12, 8, 4};
	int i;

	for (i = 0; i < (ARRAY_SIZE(supp_adcbit)); ++i) {
		if (adcbit == supp_adcbit[i])
			break;
	}
	*conf_adc_bit = (unsigned int)supp_adcbit[i];

	return isl29023_write_data(client, ISL29023_REG_ADD_COMMANDII,
		i, COMMANDII_RESOLUTION_MASK, COMMANDII_RESOLUTION_SHIFT);
}

static int isl29023_read_sensor_input(struct i2c_client *client, int mode)
{
	bool status;
	int lsb;
	int msb;

	/* Set mode */
	status = isl29023_write_data(client, ISL29023_REG_ADD_COMMAND1,
			mode, COMMMAND1_OPMODE_MASK, COMMMAND1_OPMODE_SHIFT);
	if (!status) {
		dev_err(&client->dev, "Error in setting operating mode\n");
		return -EBUSY;
	}

	msleep(CONVERSION_TIME_MS);
	lsb = i2c_smbus_read_byte_data(client, ISL29023_REG_ADD_DATA_LSB);
	if (lsb < 0) {
		dev_err(&client->dev, "Error in reading LSB DATA\n");
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, ISL29023_REG_ADD_DATA_MSB);
	if (msb < 0) {
		dev_err(&client->dev, "Error in reading MSB DATA\n");
		return msb;
	}

	dev_vdbg(&client->dev, "MSB 0x%x and LSB 0x%x\n", msb, lsb);

	return ((msb << 8) | lsb);
}

static bool isl29023_read_lux(struct i2c_client *client, int *lux)
{
	int lux_data;
	struct isl29023_chip *chip = i2c_get_clientdata(client);

	lux_data = isl29023_read_sensor_input(client, COMMMAND1_OPMODE_ALS_ONCE);
	if (lux_data > 0) {
		*lux = (lux_data * chip->range) >> chip->adc_bit;
		return true;
	}
	return false;
}

static bool isl29023_read_ir(struct i2c_client *client, int *ir)
{
	int ir_data;

	ir_data = isl29023_read_sensor_input(client, COMMMAND1_OPMODE_IR_ONCE);
	if (ir_data > 0) {
		*ir = ir_data;
		return true;
	}
	return false;
}

static ssize_t get_sensor_data(struct device *dev, char *buf, int mode)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;
	struct i2c_client *client = chip->client;
	int value = 0;
	bool status;

	mutex_lock(&chip->lock);
	switch (mode) {
		case COMMMAND1_OPMODE_ALS_ONCE:
			status = isl29023_read_lux(client, &value);
			break;
		case COMMMAND1_OPMODE_IR_ONCE:
                        status = isl29023_read_ir(client, &value);
                        break;


		default:
			dev_err(&client->dev,"Mode %d is not supported\n",mode);
			mutex_unlock(&chip->lock);
			return -EBUSY;
	}

	if (!status) {
		dev_err(&client->dev, "Error in Reading data");
		mutex_unlock(&chip->lock);
		return -EBUSY;
	}

	mutex_unlock(&chip->lock);
	return sprintf(buf, "%d\n", value);
}

/* Sysfs interface */
/* range */
static ssize_t show_range(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;

	dev_vdbg(dev, "%s()\n", __func__);
	return sprintf(buf, "%u\n", chip->range);
}

static ssize_t store_range(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;
	struct i2c_client *client = chip->client;
	bool status;
	unsigned long lval;
	unsigned int new_range;

	dev_vdbg(dev, "%s()\n", __func__);

	if (strict_strtoul(buf, 10, &lval))
		return -EINVAL;

	if (!(lval == 1000UL || lval == 4000UL ||
		lval == 16000UL || lval == 64000UL)) {
		dev_err(dev, "The range is not supported\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
	status = isl29023_set_range(client, lval, &new_range);
	if (!status) {
		mutex_unlock(&chip->lock);
		dev_err(dev, "Error in setting max range\n");
		return -EINVAL;
	}
	chip->range = new_range;
	mutex_unlock(&chip->lock);
	return count;
}

/* resolution */
static ssize_t show_resolution(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;

	dev_vdbg(dev, "%s()\n", __func__);
	return sprintf(buf, "%u\n", chip->adc_bit);
}

static ssize_t store_resolution(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;
	struct i2c_client *client = chip->client;
	bool status;
	unsigned long lval;
	unsigned int new_adc_bit;

	dev_vdbg(dev, "%s()\n", __func__);

	if (strict_strtoul(buf, 10, &lval))
		return -EINVAL;
	if (!(lval == 4 || lval == 8 || lval == 12 || lval == 16)) {
		dev_err(dev, "The resolution is not supported\n");
		return -EINVAL;
	}

	mutex_lock(&chip->lock);
	status = isl29023_set_resolution(client, lval, &new_adc_bit);
	if (!status) {
		mutex_unlock(&chip->lock);
		dev_err(dev, "Error in setting resolution\n");
		return -EINVAL;
	}
	chip->adc_bit = new_adc_bit;
	mutex_unlock(&chip->lock);
	return count;
}

/* Read lux */
static ssize_t show_lux(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	return get_sensor_data(dev, buf, COMMMAND1_OPMODE_ALS_ONCE);
}

/* Read ir */
static ssize_t show_ir(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	return get_sensor_data(dev, buf, COMMMAND1_OPMODE_IR_ONCE);
}

/* Read name */
static ssize_t show_name(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct isl29023_chip *chip = indio_dev->dev_data;
	return sprintf(buf, "%s\n", chip->client->name);
}

static IIO_DEVICE_ATTR(range, S_IRUGO | S_IWUSR, show_range, store_range, 0);
static IIO_DEVICE_ATTR(resolution, S_IRUGO | S_IWUSR,
	show_resolution, store_resolution, 0);
static IIO_DEVICE_ATTR(lux, S_IRUGO, show_lux, NULL, 0);
static IIO_DEVICE_ATTR(ir, S_IRUGO, show_ir, NULL, 0);
static IIO_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, 0);

static struct attribute *isl29023_attributes[] = {
	&iio_dev_attr_name.dev_attr.attr,
	&iio_dev_attr_range.dev_attr.attr,
	&iio_dev_attr_resolution.dev_attr.attr,
	&iio_dev_attr_lux.dev_attr.attr,
	&iio_dev_attr_ir.dev_attr.attr,
	NULL
};

static const struct attribute_group isl29108_group = {
	.attrs = isl29023_attributes,
};

static void isl29023_regulator_enable(struct i2c_client *client)
{
	struct isl29023_chip *chip = i2c_get_clientdata(client);

	chip->regulator = regulator_get(NULL, "vdd_vcore_phtn");
	if (IS_ERR_OR_NULL(chip->regulator)) {
		dev_err(&client->dev, "Couldn't get regulator vdd_vcore_phtn\n");
		chip->regulator = NULL;
	}
	else {
		regulator_enable(chip->regulator);
		/* Optimal time to get the regulator turned on
		 * before initializing isl29023 chip*/
		mdelay(5);
	}
}

static void isl29023_regulator_disable(struct i2c_client *client)
{
	struct isl29023_chip *chip = i2c_get_clientdata(client);
	struct regulator *isl29023_reg = chip->regulator;
	int ret;

	if (isl29023_reg) {
		ret = regulator_is_enabled(isl29023_reg);
		if (ret > 0)
			regulator_disable(isl29023_reg);
		regulator_put(isl29023_reg);
	}
	chip->regulator = NULL;
}

static int isl29023_chip_init(struct i2c_client *client)
{
	struct isl29023_chip *chip = i2c_get_clientdata(client);
	bool status;
	int i;
	int new_adc_bit;
	unsigned int new_range;

	isl29023_regulator_enable(client);

	for (i = 0; i < ARRAY_SIZE(chip->reg_cache); i++) {
		chip->reg_cache[i] = 0;
	}

	/* set defaults */
	status = isl29023_set_range(client, chip->range, &new_range);
	if (status)
		status = isl29023_set_resolution(client, chip->adc_bit,
				&new_adc_bit);
	if (!status) {
		dev_err(&client->dev, "Init of isl29023 fails\n");
		return -ENODEV;
	}
	return 0;
}

static int __devinit isl29023_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct isl29023_chip *chip;
	int err;

	chip = kzalloc(sizeof (struct isl29023_chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&client->dev, "Memory allocation fails\n");
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, chip);
	chip->client = client;

	mutex_init(&chip->lock);

	chip->range = 1000;
	chip->adc_bit = 16;

	err = isl29023_chip_init(client);
	if (err)
		goto exit_free;

	chip->indio_dev = iio_allocate_device();
	if (!chip->indio_dev) {
		dev_err(&client->dev, "iio allocation fails\n");
		goto exit_free;
	}

	chip->indio_dev->attrs = &isl29108_group;
	chip->indio_dev->dev.parent = &client->dev;
	chip->indio_dev->dev_data = (void *)(chip);
	chip->indio_dev->driver_module = THIS_MODULE;
	chip->indio_dev->modes = INDIO_DIRECT_MODE;
	err = iio_device_register(chip->indio_dev);
	if (err) {
		dev_err(&client->dev, "iio registration fails\n");
		goto exit_iio_free;
	}

	return 0;

exit_iio_free:
	iio_free_device(chip->indio_dev);
exit_free:
	kfree(chip);
exit:
	return err;
}

static int __devexit isl29023_remove(struct i2c_client *client)
{
	struct isl29023_chip *chip = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "%s()\n", __func__);
	isl29023_regulator_disable(client);
	iio_device_unregister(chip->indio_dev);
	kfree(chip);
	return 0;
}

static const struct i2c_device_id isl29023_id[] = {
	{"isl29023", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, isl29023_id);

static struct i2c_driver isl29023_driver = {
	.class	= I2C_CLASS_HWMON,
	.driver  = {
		.name = "isl29023",
		.owner = THIS_MODULE,
	},
	.probe	 = isl29023_probe,
	.remove  = __devexit_p(isl29023_remove),
	.id_table = isl29023_id,
};

static int __init isl29023_init(void)
{
	return i2c_add_driver(&isl29023_driver);
}

static void __exit isl29023_exit(void)
{
	i2c_del_driver(&isl29023_driver);
}

module_init(isl29023_init);
module_exit(isl29023_exit);
