/* PWM controlled fan (with one GPIO (21))
 * Runs the fan at full speed, 100% duty cycle
 *
 * Device tree bindings
 *
	mi_fan_pwm: mi_fan_pwm {
		compatible = "mi,mi-fan-pwm";
		pinctrl-names = "default", "sleep";
		pinctrl-0 = <&mi_fan_pwm_pins_default>;
		pinctrl-1 = <&mi_fan_pwm_pins_sleep>;

		fan-gpios = <&gpio 21 GPIO_ACTIVE_LOW>;

		status = "okay";
	};

	mi_fan_pwm_pins_default: mi_fan_pwm_pins_default {
		brcm,pins = <21>;
		brcm,function = <BCM2835_FSEL_GPIO_OUT>;
		brcm,pull = <BCM2835_PUD_OFF>;
	};

	mi_fan_pwm_pins_sleep: mi_fan_pwm_pins_sleep {
		brcm,pins = <21>;
		brcm,function = <BCM2835_FSEL_GPIO_IN>;
		brcm,pull = <BCM2835_PUD_UP>;
	};
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/pwm.h>
#include "mi_fan_pwm_sysfs.h"
#include "mi_fan_pwm.h"


static int mi_fan_pwm_probe(struct platform_device *pdev)
{
	struct gpio_desc *gpio_desc;
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct mi_fan_pwm_device_priv *priv;
	struct pwm_state state = { };

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;

	/* GPIO */
	gpio_desc = devm_gpiod_get_from_of_node(&pdev->dev, np, "fan-gpios", 0, GPIOD_OUT_HIGH, "fan-gpio");
	if (IS_ERR(gpio_desc))
		dev_err(&pdev->dev,
			"getting gpio failed, ret: %d\n", ret);

	priv->gpio_desc = gpio_desc;

	/* PWM */
	priv->pwm = devm_of_pwm_get(&pdev->dev, np, NULL);

	if (IS_ERR(priv->pwm))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->pwm), "Could not get PWM\n");


	dev_info(&pdev->dev, "%s: pwm period: %lld", __func__, priv->pwm->args.period);

	pwm_init_state(priv->pwm, &state);

	/* Set duty cycle to period to run the fan at full speed*/
	state.duty_cycle = priv->pwm->args.period;
	state.enabled = true;

	ret = pwm_apply_state(priv->pwm, &state);
	if (ret) {
		dev_err(&pdev->dev, "Failed to configure PWM: %d\n", ret);
		return ret;
	}

	/* SYSFS */
	ret = mi_fan_pwm_sysfs_init(pdev);
	if (ret) {
		dev_err(&pdev->dev,
			"sysfs init failed, ret: %d\n", ret);
		return ret;
	}

	pinctrl_pm_select_default_state(&pdev->dev);

	return 0;
}


static int mi_fan_pwm_remove(struct platform_device *pdev)
{
	struct mi_fan_pwm_device_priv *priv = platform_get_drvdata(pdev);

	pwm_disable(priv->pwm);

	mi_fan_pwm_sysfs_remove(pdev);

    return 0;
}


int mi_fan_pwm_suspend(struct device *dev)
{
	int ret;
	struct pwm_args args;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_fan_pwm_device_priv *priv = platform_get_drvdata(pdev);

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
	*/
	if (pm_suspend_target_state == PM_SUSPEND_MEM || pm_suspend_target_state == PM_SUSPEND_TO_IDLE){
		dev_info(dev, "%s: suspending to RAM", __func__);

		pinctrl_pm_select_sleep_state(dev);
	}else{
		dev_info(dev, "%s: suspending, standby", __func__);
	}

	pwm_get_args(priv->pwm, &args);

	ret = pwm_config(priv->pwm, 0, args.period);
	if (ret < 0)
		return ret;

	pwm_disable(priv->pwm);

	return 0;
}

int mi_fan_pwm_resume(struct device *dev)
{
	int ret;
	struct pwm_args args;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_fan_pwm_device_priv *priv = platform_get_drvdata(pdev);

	dev_info(dev, "%s: resuming", __func__);

	pwm_get_args(priv->pwm, &args);
	ret = pwm_config(priv->pwm, args.period, args.period);
	if (ret)
		return ret;
	return pwm_enable(priv->pwm);

	if (pm_suspend_target_state == PM_SUSPEND_MEM || pm_suspend_target_state == PM_SUSPEND_TO_IDLE)
		pinctrl_pm_select_default_state(dev);

	return 0;
}

static struct dev_pm_ops dev_pm_ops = {
        .resume        = mi_fan_pwm_resume,
        .suspend       = mi_fan_pwm_suspend,
};

static const struct of_device_id mi_fan_pwm_of_match[] = {
	{ .compatible = "mi,mi-fan-pwm", },
	{}
};
MODULE_DEVICE_TABLE(of, mi_fan_pwm_of_match);

/*
 * bind and unbind driver manually from userspace:
 * echo mi_fan_pwm > /sys/bus/platform/drivers/mi_fan_pwm/bind
 * echo mi_fan_pwm > /sys/bus/platform/drivers/mi_fan_pwm/unbind
 */
static const struct platform_device_id mi_fan_pwm_id_table[] = {
	{"mi_fan_pwm"},
	{},
};

static struct platform_driver mi_fan_pwm_driver = {
	.probe = mi_fan_pwm_probe,
	.remove = mi_fan_pwm_remove,
	.id_table = mi_fan_pwm_id_table,
	.driver = {
		.name = "mi_fan_pwm",
		.of_match_table = mi_fan_pwm_of_match,
		.pm = &dev_pm_ops,
	},
};
module_platform_driver(mi_fan_pwm_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("rpi fan control module.");
MODULE_VERSION("0.01");