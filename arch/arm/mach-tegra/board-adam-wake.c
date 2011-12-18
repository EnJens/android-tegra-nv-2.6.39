/*
 *  Copyright (c) 1999-2001 Vojtech Pavlik
 */

/*
 *  Input driver event debug module - dumps all events into syslog
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/device.h>

#include <linux/mfd/tps6586x.h>

MODULE_AUTHOR("Dane Wagner <dane.wagner@gmail.com>");
MODULE_DESCRIPTION("Wake adam when power button is pressed");
MODULE_LICENSE("GPL");

static void adam_wake_do_wake(struct work_struct *work) {
	// Need to queue this function since i2c is slow and this handler
	// should be atomic.
	tps6586x_cancel_sleep();
}
DECLARE_WORK(do_wake, adam_wake_do_wake);

static void adam_wake_force_shutdown(struct work_struct *work) {
	// Need to queue this function since i2c is slow and this handler
	// should be atomic.
	tps6586x_power_off();
}
DECLARE_DELAYED_WORK(do_shutdown, adam_wake_force_shutdown);

static void adam_wake_event(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
	code = code;
	if(type == EV_KEY && code == KEY_POWER) {
	  if(!!value) {
	    // button pressed; cancel TPS sleep mode
	    schedule_work(&do_wake);
	    // ... and schedule shutdown if button remains pressed
	    schedule_delayed_work(&do_shutdown, 5*HZ);
	  } else {
	    // button is not longer pressed; cancel shutdown
	    cancel_delayed_work(&do_shutdown);
	  }
	}
}

static int adam_wake_connect(struct input_handler *handler, struct input_dev *dev,
			 const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "adam_wake";

	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;

	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;

	printk(KERN_DEBUG "adam_wake.c: Connected device: %s (%s at %s)\n",
		dev_name(&dev->dev),
		dev->name ?: "unknown",
		dev->phys ?: "unknown");

	return 0;

 err_unregister_handle:
	input_unregister_handle(handle);
 err_free_handle:
	kfree(handle);
	return error;
}

static void adam_wake_disconnect(struct input_handle *handle)
{
	printk(KERN_DEBUG "adam_wake.c: Disconnected device: %s\n",
		dev_name(&handle->dev->dev));

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id adam_wake_ids[] = {
  { .driver_info = 1,
    .vendor = 0x0001,
    .product = 0x0001,
    .version = 0x0100,
  },
  { },			/* Terminating zero entry */
};

MODULE_DEVICE_TABLE(input, adam_wake_ids);

static bool adam_wake_match(struct input_handler *handler,
			       struct input_dev *dev) {
  // TODO: look for EV_KEY: KEY_POWER capability
  return dev->name && strcmp("gpio-keys", dev->name) == 0;
}

static struct input_handler adam_wake_handler = {
	.event =	adam_wake_event,
	.connect =	adam_wake_connect,
	.disconnect =	adam_wake_disconnect,
	.name =		"adam_wake",
	.id_table =	adam_wake_ids,
	.match =        adam_wake_match,
};

static int __init adam_wake_init(void)
{
	return input_register_handler(&adam_wake_handler);
}

static void __exit adam_wake_exit(void)
{
	input_unregister_handler(&adam_wake_handler);
}

module_init(adam_wake_init);
module_exit(adam_wake_exit);
