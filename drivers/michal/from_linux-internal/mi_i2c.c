#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/regmap.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/pinctrl/consumer.h>
#include "mi_i2c.h"
#include "mi_i2c_sysfs.h"
#include "mi_i2c_test.h"

static const struct regmap_config mi_i2c_regmap_cfg = {
	.reg_bits = 8,
	.val_bits = 8,
	.name = "mi_i2c",
};

static int mi_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct mi_i2c_device *mi_i2c_device;

	dev_info(&client->dev, "%s: ", __func__);

	/* Managed kzalloc. Memory allocated with this function is automatically
	 * freed on driver detach. Like all other devres resources,
	 * guaranteed alignment is unsigned long long.
	 */
	mi_i2c_device = devm_kzalloc(&client->dev, sizeof(struct mi_i2c_device), GFP_KERNEL);
	if (mi_i2c_device == NULL)
		return -ENOMEM;

	mi_i2c_device->dev = &client->dev;

	/* Each client structure has a special data field that can point to
	 * any structure at all. You should use this to keep device-specific data.
	 */
	i2c_set_clientdata(client, mi_i2c_device);

	ret = mi_i2c_sysfs_init(client);
	if (ret)
		return ret;

	mi_i2c_transfer_test(client);

	mi_i2c_device->regmap = devm_regmap_init_i2c(client, &mi_i2c_regmap_cfg);
	if (IS_ERR(mi_i2c_device->regmap)) {
		ret = PTR_ERR(mi_i2c_device->regmap);
		dev_err(mi_i2c_device->dev,
			"Failed to initialize register map: %d\n", ret);
		return ret;
	}
	return 0;
}

static int mi_i2c_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "%s: ", __func__);

	mi_i2c_sysfs_remove(client);

	return 0;
}

/* When an I2C device is instantiated via OF device tree,
 * its compatible property is matched (format: "manufacturer,model")
 * and the "model" component is matched against the struct i2c_device_id array
 * - which also passed that element to mi_i2c_probe.
 */
static const struct of_device_id mi_i2c_of_match[] = {
	{ .compatible = "mi,mi_i2c", },
	{}
};
MODULE_DEVICE_TABLE(of, mi_i2c_of_match);

/* When an I2C device is instantiated via userspace:
 * echo mi_i2c_device 0x50 > /sys/bus/i2c/devices/i2c-3/new_device
 * The i2c-core matches the name "mi_i2c_device" to the name in the
 * struct i2c_device_id array, and it passes that element to mi_i2c_probe.
 * echo mi_i2c_device 0x50 > /sys/bus/i2c/devices/i2c-3/new_device
 * calls mi_i2c_remove
 *
 * bind and unbind driver manually from userspace:
 * echo 3-0050 >  /sys/module/mi_i2c/drivers/i2c\:mi_i2c/bind
 * echo 3-0050 >  /sys/module/mi_i2c/drivers/i2c\:mi_i2c/unbind
 */
 static const struct i2c_device_id mi_i2c_table[] = {
	{"mi_i2c"},
	{},
};

int mi_i2c_suspend(struct device *dev)
{
	struct i2c_client *client;

	client = container_of(dev, struct i2c_client, dev);

	/*	Although the driver core handles selection of the default state
	* during the initial probe of the driver, some extra work may be
	* needed within the driver to make sure the sleep state is selected
	* during suspend and the default state is re-selected at resume time.
	* This is accomplished by placing calls to pinctrl_pm_select_sleep_state
	* at the end of the suspend handler of the driver and pinctrl_pm_select_default_state
	* at the start of the resume handler. These functions will not cause failure
	* if the driver cannot find a sleep state so even with them added
	* the sleep state is still default.
	* Some drivers rely on the default configuration of the pins without
	* any need for a default pinctrl entry to be set but if a sleep state
	* is added a default state must be added as well in order for the resume
	* path to be able to properly reconfigure the pins.
	*
	* epd_pmic: mi_i2c@62 {
	*	compatible = "mi,mi_i2c";
	*	reg = <0x62>;
	*	status = "okay";
	*	pinctrl-names = "default", "sleep";   <------
	*	pinctrl-0 = <&pinctrl_epdpmic>;       <------ same pin state for default and sleep
	*	pinctrl-1 = <&pinctrl_epdpmic>;       <------ in this case.
	* };

	* pinctrl_epdpmic: epdpmicgrp {
	*	fsl,pins = <
	*		MX7D_PAD_SAI2_RX_DATA__GPIO6_IO21 0x00000074
	*		MX7D_PAD_ENET1_RGMII_TXC__GPIO7_IO11 0x00000014
	*		MX7D_PAD_SAI1_MCLK__GPIO6_IO18 0x00000074
	*	>;
	*};
	*/

	if (pm_suspend_target_state == PM_SUSPEND_MEM){
		dev_info(dev, "%s: suspending to RAM", __func__);

		pinctrl_pm_select_sleep_state(dev);
	}else{
		dev_info(dev, "%s: suspending, standby", __func__);

		if (device_can_wakeup(dev)){
			dev_info(dev, "%s: device can wakeup", __func__);

			enable_irq_wake(client->irq);
		}else{
			dev_info(dev, "%s: device cannot wakeup", __func__);
		}
	}

	return 0;
}

int mi_i2c_resume(struct device *dev)
{
	struct i2c_client *client;

	client = container_of(dev, struct i2c_client, dev);

	dev_info(dev, "%s: resuming", __func__);

	if (device_may_wakeup(dev))
		disable_irq_wake(client->irq);

	if (pm_suspend_target_state == PM_SUSPEND_MEM)
		pinctrl_pm_select_default_state(dev);

	return 0;
}

static struct dev_pm_ops dev_pm_ops = {
        .resume        = mi_i2c_resume,
        .suspend       = mi_i2c_suspend,
};

static struct i2c_driver mi_i2c_driver = {
	.driver = {
		   .name = "mi_i2c",		   
		   .of_match_table = of_match_ptr(mi_i2c_of_match),
		   .pm = &dev_pm_ops,
		   },
	.id_table = mi_i2c_table,
	.probe = mi_i2c_probe,
	.remove = mi_i2c_remove,
};

/* Helper macro for I2C drivers which do not do anything special in module init/exit. 
 * This eliminates a lot of boilerplate. Each module may only use this macro once, 
 * and calling it replaces module_init and module_exit
 */
module_i2c_driver(mi_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("A simple example Linux i2c module.");
MODULE_VERSION("0.01");