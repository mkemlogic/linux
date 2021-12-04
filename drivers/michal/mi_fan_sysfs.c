#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "mi_fan_sysfs.h"
#include "mi_fan.h"


static ssize_t fan_gpio_value_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_fan_device_priv *priv = platform_get_drvdata(pdev);

	if (buf[0] == '0') {
		gpio_set_value(priv->gpio, 0);
		dev_info(dev, "%s: gpio:%d, value:%d", __func__, priv->gpio, 0);
	}

	if (buf[0] == '1') {
		gpio_set_value(priv->gpio, 1);
		dev_info(dev, "%s: gpio:%d, value:%d", __func__, priv->gpio, 1);
	}

	return count;
}

static DEVICE_ATTR(fan_gpio_value, 0644, NULL, fan_gpio_value_store);

static struct attribute *mi_fan_attrs[] = {
	&dev_attr_fan_gpio_value.attr,	
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