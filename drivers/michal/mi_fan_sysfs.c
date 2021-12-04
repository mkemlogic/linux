#include <linux/module.h>
#include <linux/sysfs.h>
#include "mi_fan_sysfs.h"


static ssize_t fan_gpio_value_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
/*	int ret;
	struct mi_fan_device *mi_fan_device = dev_get_drvdata(dev);

	ret = hex2bin(&mi_fan_device->reg_addr, buf, 2);
	dev_info(dev, "0x%02x", mi_fan_device->reg_addr);

	return count;*/
    return 0;
}


static DEVICE_ATTR(fan_gpio_value, 0644, NULL, fan_gpio_value_store);


static struct attribute *mi_fan_attrs[] = {
	&dev_attr_fan_gpio_value.attr,	
	NULL,
};

static const struct attribute_group mi_fan_attrs_group = {
	.attrs = mi_fan_attrs,
};

int mi_fan_sysfs_init(struct platform_device *pdev)
{
    int ret;
	ret = sysfs_create_group(&pdev->dev.kobj, &mi_fan_attrs_group);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}
    return ret;
}
EXPORT_SYMBOL(mi_fan_sysfs_init);

void mi_fan_sysfs_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &mi_fan_attrs_group);
}
EXPORT_SYMBOL(mi_fan_sysfs_remove);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("rpi fan control module.");
MODULE_VERSION("0.01");