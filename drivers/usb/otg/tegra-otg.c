/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
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

#include <linux/usb.h>
#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/platform_device.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/wakelock.h>

#define TEGRA_USB_PHY_WAKEUP_REG_OFFSET		0x408

#define  TEGRA_ID_INT_EN		(1 << 0)
#define  TEGRA_ID_INT_STATUS	(1 << 1)
#define  TEGRA_ID_STATUS		(1 << 2)
#define  TEGRA_ID_WAKEUP_EN		(1 << 6)
#define  TEGRA_ID_SW_VALUE		(1 << 4)
#define  TEGRA_ID_SW_ENABLE		(1 << 3)

#define  TEGRA_VBUS_INT_EN		(1 << 8)
#define  TEGRA_VBUS_INT_STATUS	(1 << 9)
#define  TEGRA_VBUS_STATUS		(1 << 10)
#define  TEGRA_VBUS_WAKEUP_EN	(1 << 30)
#define  TEGRA_VBUS_SW_VALUE	(1 << 12)
#define  TEGRA_VBUS_SW_ENABLE	(1 << 11)

#define  TEGRA_INTS		(TEGRA_VBUS_INT_STATUS | TEGRA_ID_INT_STATUS)

struct tegra_otg_data {
	struct otg_transceiver 	otg;
	spinlock_t 				lock;
	struct wake_lock 		wake_lock;
	unsigned long 			event;
	bool 					host_mode;
	void __iomem *			regs;
	struct clk *			clk;
	int						irq;
	struct platform_device *pdev;
	struct work_struct 		work;
	unsigned int 			intr_reg_data;
	
};

static struct tegra_otg_data *tegra_clone = NULL;

static inline unsigned long otg_readl(struct tegra_otg_data *tegra,
				      unsigned int offset)
{
	return readl(tegra->regs + offset);
}

static inline void otg_writel(struct tegra_otg_data *tegra, unsigned long val,
			      unsigned int offset)
{
	writel(val, tegra->regs + offset);
}

static const char *tegra_state_name(enum usb_otg_state state)
{
	if (state == OTG_STATE_A_HOST)
		return "HOST";
	if (state == OTG_STATE_B_PERIPHERAL)
		return "PERIPHERAL";
	if (state == OTG_STATE_A_SUSPEND)
		return "SUSPEND";
	return "INVALID";
}

static void tegra_start_host(struct tegra_otg_data *tegra)
{
	struct tegra_otg_platform_data *pdata = tegra->otg.dev->platform_data;
	if (!tegra->pdev) {
		tegra->pdev = pdata->host_register();
	}
}

static void tegra_stop_host(struct tegra_otg_data *tegra)
{
	struct tegra_otg_platform_data *pdata = tegra->otg.dev->platform_data;
	if (tegra->pdev) {
		pdata->host_unregister(tegra->pdev);
		tegra->pdev = NULL;
	}
}

static void tegra_otg_work(struct work_struct *work)
{
	struct tegra_otg_data *tegra = container_of(work, struct tegra_otg_data, work);
	struct otg_transceiver *otg = &tegra->otg;
	enum usb_otg_state from = otg->state;
	enum usb_otg_state to = OTG_STATE_UNDEFINED;
	unsigned long event = tegra->event;
	
	unsigned long val;
	unsigned long flags;
		
	if (event == USB_EVENT_VBUS)
		to = OTG_STATE_B_PERIPHERAL;
	else if (event == USB_EVENT_ID)
		to = OTG_STATE_A_HOST;
	else
		to = OTG_STATE_A_SUSPEND;

	if (from == to)
		return;
		
	otg->state = to;

	dev_info(tegra->otg.dev, "%s --> %s", tegra_state_name(from),
					      tegra_state_name(to));
	dev_info(tegra->otg.dev, "host: %p, gadget: %p", otg->host, otg->gadget);
						  
	clk_enable(tegra->clk);

	if ((to == OTG_STATE_A_HOST) && (from == OTG_STATE_B_PERIPHERAL)
			&& otg->gadget) {

		usb_gadget_vbus_disconnect(otg->gadget);
		wake_unlock(&tegra->wake_lock);
		
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val &= ~TEGRA_ID_SW_VALUE;			/* CableID is 0 */
		val &= ~TEGRA_VBUS_INT_EN;			/* We can't use VBUS ints in host mode */
		
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

		tegra_start_host(tegra);

	} else if ((to == OTG_STATE_A_HOST) && (from == OTG_STATE_A_SUSPEND)) {

		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val &= ~TEGRA_ID_SW_VALUE;			/* CableID is 0 */
		val &= ~TEGRA_VBUS_INT_EN;			/* We can't use VBUS ints in host mode */
		
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

		tegra_start_host(tegra);

	} else if ((to == OTG_STATE_A_SUSPEND) && (from == OTG_STATE_A_HOST)) {
		
		tegra_stop_host(tegra);
		
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val |=  TEGRA_ID_SW_VALUE;			/* CableID is 1 */
		val |=  TEGRA_VBUS_INT_EN;			/* Use VBUS ints in suspend mode to wakeup */
		
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

	} else if ((to == OTG_STATE_B_PERIPHERAL) && (from == OTG_STATE_A_HOST)
			&& otg->gadget) {

		tegra_stop_host(tegra);	
		
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val |=  TEGRA_ID_SW_VALUE;			/* CableID is 1 */
		val |=  TEGRA_VBUS_INT_EN;			/* Use VBUS ints in suspend mode to wakeup */

		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

		wake_lock(&tegra->wake_lock);
		usb_gadget_vbus_connect(otg->gadget);
		
	} else if ((to == OTG_STATE_B_PERIPHERAL) && (from == OTG_STATE_A_SUSPEND)
			&& otg->gadget) {
			
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val |=  TEGRA_ID_SW_VALUE;			/* CableID is 1 */
		val |=  TEGRA_VBUS_INT_EN;			/* Use VBUS ints in suspend mode to wakeup */

		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

		wake_lock(&tegra->wake_lock);
		usb_gadget_vbus_connect(otg->gadget);

	} else if ((to == OTG_STATE_A_SUSPEND) && (from == OTG_STATE_B_PERIPHERAL)
			&& otg->gadget) {

		usb_gadget_vbus_disconnect(otg->gadget);
		wake_unlock(&tegra->wake_lock);
		
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		
		val |=  TEGRA_ID_SW_VALUE;			/* CableID is 1 */
		val |=  TEGRA_VBUS_INT_EN;			/* Use VBUS ints in suspend mode to wakeup */
		
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);

	}

	clk_disable(tegra->clk);
}

static irqreturn_t tegra_otg_irq(int irq, void *data)
{
	struct tegra_otg_data *tegra = data;
	unsigned long val;
	unsigned long flags;
	
	/* Get the interrupt cause and clean it - Clocks are already enabled. 
	   Don't try to enable them here, as those functions can sleep and that
	   is not allowed in an interrupt ISR */
	spin_lock_irqsave(&tegra->lock, flags);
	val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	spin_unlock_irqrestore(&tegra->lock, flags);

	/* We are interested in the VBUS change, as the cableID is
	   not usable */
	
	/* If VBUS interrupts are enabled and we are in gadget mode and a change in VBUS detection is found */
	if ((val & TEGRA_VBUS_INT_EN) && 
		!tegra->host_mode && 
		(val & TEGRA_VBUS_INT_STATUS)) {
		
		/* Based on the VBUS status, switch to the right mode */
		unsigned long event = (val & TEGRA_VBUS_STATUS) ? USB_EVENT_VBUS : 0;
		
		/* If an event change was detected */
		if (tegra->event != event) {
		
			/* Store the new event */
			tegra->event = event;
			
			/* Schedule work */
			schedule_work(&tegra->work);
		}
	}
	
	return IRQ_HANDLED;
}

static void tegra_otg_check_status(void)
{
	unsigned long val;
	struct tegra_otg_data *tegra = tegra_clone;
	if (!tegra) 
		return;
		
	/* Read the cable status - No need to lock here */
	clk_enable(tegra->clk);
	val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	clk_disable(tegra->clk);

	/* We are interested in the VBUS change, as the cableID is not usable */
	/* If VBUS interrupts are enabled and we are in gadget mode... */
	if ((val & TEGRA_VBUS_INT_EN) && !tegra->host_mode) {

		/* Based on the VBUS status, switch to the right mode */
		unsigned long event = (val & TEGRA_VBUS_STATUS) ? USB_EVENT_VBUS : 0;
		
		/* If an event change was detected */
		if (tegra->event != event) {
		
			/* Store the new event */
			tegra->event = event;
			
			/* Schedule work */
			schedule_work(&tegra->work);
		}
	}

}

void tegra_otg_check_vbus_detection(void)
{
	tegra_otg_check_status();
}
EXPORT_SYMBOL(tegra_otg_check_vbus_detection); 

/* Switch interface from host to slave and viceversa */
void tegra_otg_set_host_mode(bool host_mode)
{
	unsigned long val;
	unsigned long flags;	

	struct tegra_otg_data *tegra = tegra_clone;
	if (!tegra) 
		return;

	dev_info(tegra->otg.dev, "%s: mode: %s", __func__, host_mode ? "host" : "gadget");
		
	if (tegra->host_mode == host_mode)
		return;
		
	/* Store the new mode */
	tegra->host_mode = host_mode;

	if (host_mode) {
	
		/* No interrupts can be used in host mode */
		clk_enable(tegra->clk);
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; 				/* Do NOT ack pending interrupts */
		val &= ~TEGRA_ID_SW_VALUE;			/* CableID is 0 */
		val &= ~TEGRA_VBUS_INT_EN;			/* We can't use VBUS ints in host mode */
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);		
		clk_disable(tegra->clk);

		/* Schedule the switch to host mode */
		if (tegra->event != USB_EVENT_ID) {
			tegra->event = USB_EVENT_ID;
			schedule_work(&tegra->work);
		}
		
	} else {
	
		/* Enable the VBUS id interrupts, so we will know when they disconnected */
		clk_enable(tegra->clk);
		spin_lock_irqsave(&tegra->lock, flags);
		val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		val &= ~TEGRA_INTS; /* Do NOT ack pending interrupts */
		val |=  TEGRA_ID_SW_VALUE;			/* CableID is 1 */
		val |=  TEGRA_VBUS_INT_EN;			/* Use VBUS ints in suspend mode to wakeup */
		otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
		spin_unlock_irqrestore(&tegra->lock, flags);		
		clk_disable(tegra->clk);
	
		/* Leave some time for VBUS stabilization */
		mdelay(10);
		
		/* Check if we are in device mode with something plugged to us */
		tegra_otg_check_status();
	}
}
EXPORT_SYMBOL(tegra_otg_set_host_mode);


static int tegra_otg_set_peripheral(struct otg_transceiver *otg,
				struct usb_gadget *gadget)
{
	dev_info(otg->dev, "%s: gadget: %p", __func__, gadget);
	otg->gadget = gadget;
	
	/* Force the checking of the state, in case we missed it */
	if (gadget) {
		tegra_otg_check_status();
	}

	return 0;
}

static int tegra_otg_set_host(struct otg_transceiver *otg,
				struct usb_bus *host)
{
	dev_info(otg->dev, "%s: host: %p", __func__, host);
	otg->host = host;

	/* Force the checking of the state, in case we missed it */
	if (host) {
		tegra_otg_check_status();
	}

	return 0;
}

static int tegra_otg_set_power(struct otg_transceiver *otg, unsigned mA)
{
	return 0;
}

static int tegra_otg_set_suspend(struct otg_transceiver *otg, int suspend)
{
	return 0;
}

static int tegra_otg_probe(struct platform_device *pdev)
{
	struct tegra_otg_data *tegra;
	struct resource *res;
	unsigned long val;
	int err;
	struct tegra_otg_platform_data *pdata = pdev->dev.platform_data;
	
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data\n");
		return -ENODEV;
	}

	tegra = kzalloc(sizeof(struct tegra_otg_data), GFP_KERNEL);
	if (!tegra)
		return -ENOMEM;

	tegra->otg.dev = &pdev->dev;
	tegra->otg.label = "tegra-otg";
	tegra->otg.state = OTG_STATE_UNDEFINED;
	tegra->otg.set_host = tegra_otg_set_host;
	tegra->otg.set_peripheral = tegra_otg_set_peripheral;
	tegra->otg.set_suspend = tegra_otg_set_suspend;
	tegra->otg.set_power = tegra_otg_set_power;
	spin_lock_init(&tegra->lock);
	wake_lock_init(&tegra->wake_lock, WAKE_LOCK_SUSPEND, "tegra_otg");

	platform_set_drvdata(pdev, tegra);
	tegra_clone = tegra;

	tegra->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(tegra->clk)) {
		dev_err(&pdev->dev, "Can't get otg clock\n");
		err = PTR_ERR(tegra->clk);
		goto err_clk;
	}

	/* Enable the clock and leave it enabled */
	err = clk_enable(tegra->clk);
	if (err)
		goto err_clken;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get I/O memory\n");
		err = -ENXIO;
		goto err_io;
	}
	tegra->regs = ioremap(res->start, resource_size(res));
	if (!tegra->regs) {
		err = -ENOMEM;
		goto err_io;
	}
	
	/* We start in a suspended device mode */
	tegra->otg.state = OTG_STATE_A_SUSPEND;

	val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	val &= ~TEGRA_ID_WAKEUP_EN;							/* Disable wakeup on CableID change */
	val |=  TEGRA_VBUS_WAKEUP_EN; 						/* Enable Phy wakeup on VBUS change */
	val |=  TEGRA_ID_SW_ENABLE;							/* Enable CableID sw overrride */
	val &= ~TEGRA_VBUS_SW_ENABLE;						/* Disable VBUS   sw overrride */
	val |=  TEGRA_ID_SW_VALUE;							/* CableID = 1, for peripheral mode */
	val &= ~TEGRA_VBUS_SW_VALUE;						/* Clean VBUS sw value */
	val |=  TEGRA_VBUS_INT_EN;							/* Enable VBUS change interrupt */
	val &= ~TEGRA_ID_INT_EN;							/* Disable cable ID change interrupt */
	otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);

	err = otg_set_transceiver(&tegra->otg);
	if (err) {
		dev_err(&pdev->dev, "can't register transceiver (%d)\n", err);
		goto err_otg;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		err = -ENXIO;
		goto err_irq;
	}
	tegra->irq = res->start;
	err = request_threaded_irq(tegra->irq, tegra_otg_irq,
				NULL,
				IRQF_SHARED, "tegra-otg", tegra);
	if (err) {
		dev_err(&pdev->dev, "Failed to register IRQ\n");
		goto err_irq;
	}

	INIT_WORK (&tegra->work, tegra_otg_work);

	dev_info(&pdev->dev, "otg transceiver registered\n");
	return 0;

err_irq:
	otg_set_transceiver(NULL);
err_otg:
	iounmap(tegra->regs);
err_io:
	clk_disable(tegra->clk);
err_clken:
	clk_put(tegra->clk);
err_clk:
	wake_lock_destroy(&tegra->wake_lock);
	platform_set_drvdata(pdev, NULL);
	kfree(tegra);
	return err;
}

static int __exit tegra_otg_remove(struct platform_device *pdev)
{
	struct tegra_otg_data *tegra = platform_get_drvdata(pdev);

	free_irq(tegra->irq, tegra);
	otg_set_transceiver(NULL);
	iounmap(tegra->regs);
	clk_disable(tegra->clk);
	clk_put(tegra->clk);
	wake_lock_destroy(&tegra->wake_lock);
	platform_set_drvdata(pdev, NULL);
	kfree(tegra);

	return 0;
}

#ifdef CONFIG_PM
static int tegra_otg_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_otg_data *tegra = platform_get_drvdata(pdev);
			
	/* store the interrupt enable for cable ID and VBUS */
	clk_enable(tegra->clk);
	tegra->intr_reg_data = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	clk_disable(tegra->clk);		
	
	/* This disables the setup enabled clk */
	clk_disable(tegra->clk);

	return 0;
}

static void tegra_otg_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_otg_data *tegra = platform_get_drvdata(pdev);
	int val;
	unsigned long flags;

	clk_enable(tegra->clk);
	
	/* Following delay is intentional.
	 * It is placed here after observing system hang.
	 * Root cause is not confirmed.
	 */
	msleep(1);
	
	clk_enable(tegra->clk);
	spin_lock_irqsave(&tegra->lock, flags);
	val = otg_readl(tegra, TEGRA_USB_PHY_WAKEUP_REG_OFFSET) | tegra->intr_reg_data;
	otg_writel(tegra, val, TEGRA_USB_PHY_WAKEUP_REG_OFFSET);
	spin_unlock_irqrestore(&tegra->lock, flags);		
	clk_disable(tegra->clk);
	
	/* Check if we are in device mode with something plugged to us */
	tegra_otg_check_status();
}

static const struct dev_pm_ops tegra_otg_pm_ops = {
	.complete = tegra_otg_resume,
	.suspend = tegra_otg_suspend,
};
#endif


static struct platform_driver tegra_otg_driver = {
	.driver = {
		.name  = "tegra-otg",
#ifdef CONFIG_PM
		.pm    = &tegra_otg_pm_ops,
#endif
	},
	.remove  = __exit_p(tegra_otg_remove),
	.probe   = tegra_otg_probe,
};

static int __init tegra_otg_init(void)
{
	return platform_driver_register(&tegra_otg_driver);
}
subsys_initcall(tegra_otg_init);

static void __exit tegra_otg_exit(void)
{
	platform_driver_unregister(&tegra_otg_driver);
}
module_exit(tegra_otg_exit);
