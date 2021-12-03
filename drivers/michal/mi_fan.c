#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/sysfs.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/pinctrl/consumer.h>



#if 0
static int mi_fan_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{

	int ret;
	struct mi_fan_device *mi_fan_device;

	dev_info(&client->dev, "%s: ", __func__);

	/* Managed kzalloc. Memory allocated with this function is automatically
	 * freed on driver detach. Like all other devres resources,
	 * guaranteed alignment is unsigned long long.
	 */
	mi_fan_device = devm_kzalloc(&client->dev, sizeof(struct mi_fan_device), GFP_KERNEL);
	if (mi_fan_device == NULL)
		return -ENOMEM;

	mi_fan_device->dev = &client->dev;

	/* Each client structure has a special data field that can point to
	 * any structure at all. You should use this to keep device-specific data.
	 */
	i2c_set_clientdata(client, mi_fan_device);

	ret = mi_fan_sysfs_init(client);
	if (ret)
		return ret;

	mi_fan_transfer_test(client);

	mi_fan_device->regmap = devm_regmap_init_i2c(client, &mi_fan_regmap_cfg);
	if (IS_ERR(mi_fan_device->regmap)) {
		ret = PTR_ERR(mi_fan_device->regmap);
		dev_err(mi_fan_device->dev,
			"Failed to initialize register map: %d\n", ret);
		return ret;
	}
	return 0;

}

static int mi_fan_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "%s: ", __func__);

	mi_fan_sysfs_remove(client);

	return 0;
}

#endif


static int mi_fan_probe(struct platform_device *platform)
{
	return 0;
}


static int mi_fan_remove(struct platform_device *platform)
{
    return 0;
}


static const struct of_device_id mi_fan_of_match[] = {
	{ .compatible = "mi,mi_fan", },
	{}
};
MODULE_DEVICE_TABLE(of, mi_fan_of_match);


int mi_fan_suspend(struct device *dev)
{
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
	* epd_pmic: mi_fan@62 {
	*	compatible = "mi,mi_fan";
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
	}

	return 0;
}

int mi_fan_resume(struct device *dev)
{
	dev_info(dev, "%s: resuming", __func__);

	if (pm_suspend_target_state == PM_SUSPEND_MEM)
		pinctrl_pm_select_default_state(dev);

	return 0;
}

static struct dev_pm_ops dev_pm_ops = {
        .resume        = mi_fan_resume,
        .suspend       = mi_fan_suspend,
};

static struct platform_driver mi_fan_driver = {
	.driver = {
		   .name = "mi_fan",
		   .of_match_table = of_match_ptr(mi_fan_of_match),
		   .pm = &dev_pm_ops,
		   },
	.probe = mi_fan_probe,
	.remove = mi_fan_remove,
};

module_platform_driver(mi_fan_driver);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("A simple example Linux i2c module.");
MODULE_VERSION("0.01");