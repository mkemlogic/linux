#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include "mi_fan_sysfs.h"
#include "mi_fan.h"


static ssize_t fan_gpio_value_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_fan_device_priv *priv = platform_get_drvdata(pdev);

	if (buf[0] == '0') {
		gpiod_set_value(priv->gpio_desc, 0);
	}

	if (buf[0] == '1') {
		gpiod_set_value(priv->gpio_desc, 1);
	}

	return count;
}

static DEVICE_ATTR(fan_gpio_value, 0644, NULL, fan_gpio_value_store);

static ssize_t fan_pwm_duty_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_fan_device_priv *priv = platform_get_drvdata(pdev);
	unsigned long duty;
	int ret;

	ret = kstrtol(buf, 10, &duty);
	if (ret < 0)
		return count;

	if (duty > priv->pwm->args.period) {
		dev_info(&pdev->dev, "Max duty cycle is: %lld\n", priv->pwm->args.period);
		return count;
	}

	dev_info(&pdev->dev, "New duty cycle: %ld\n", duty);

	pwm_config(priv->pwm, duty, priv->pwm->args.period);

	return count;

}

static DEVICE_ATTR(fan_pwm_duty, 0644, NULL, fan_pwm_duty_store);

static struct attribute *mi_fan_attrs[] = {
	&dev_attr_fan_gpio_value.attr,
	&dev_attr_fan_pwm_duty.attr,
	NULL,
};

ATTRIBUTE_GROUPS(mi_fan);

int mi_fan_sysfs_init(struct platform_device *pdev)
{
	int ret;
	ret = sysfs_create_groups(&pdev->dev.kobj, mi_fan_groups);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}
	return ret;
}
EXPORT_SYMBOL(mi_fan_sysfs_init);

void mi_fan_sysfs_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &mi_fan_group);
}
EXPORT_SYMBOL(mi_fan_sysfs_remove);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("rpi fan control module.");
MODULE_VERSION("0.01");