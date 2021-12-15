#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/i2c.h>
#include "mi_i2c.h"


static ssize_t reg_addr_show(struct device *dev, struct device_attribute *attr,
                        char *buf)
{
	struct mi_i2c_device *mi_i2c_device = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "0x%02x\n", mi_i2c_device->reg_addr);
}

static ssize_t reg_addr_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	int ret;
	struct mi_i2c_device *mi_i2c_device = dev_get_drvdata(dev);

	ret = hex2bin(&mi_i2c_device->reg_addr, buf, 2);
	dev_info(dev, "0x%02x", mi_i2c_device->reg_addr);

	return count;
}

/* Set register address by writing to reg_addr, then read register value by
 * reading from reg_value
 */
static ssize_t reg_value_show(struct device *dev, struct device_attribute *attr,
                        char *buf)
{
	int ret;
	unsigned int value;
	struct mi_i2c_device *mi_i2c_device = dev_get_drvdata(dev);

	ret = regmap_read(mi_i2c_device->regmap, mi_i2c_device->reg_addr, &value);
	if(ret)
		return scnprintf(buf, PAGE_SIZE, "failed reading register at address  0x%02x\n",
						mi_i2c_device->reg_value);
	else
		mi_i2c_device->reg_value = (value & 0xFF);

	return scnprintf(buf, PAGE_SIZE, "0x%02x\n", mi_i2c_device->reg_value);
}

/* Set register address by writing to reg_addr, then set register value by
 * writing to reg_value
 */
static ssize_t reg_value_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	int ret;
	struct mi_i2c_device *mi_i2c_device = dev_get_drvdata(dev);

	ret = hex2bin(&mi_i2c_device->reg_value, buf, 2);
	dev_info(dev, "0x%02x", mi_i2c_device->reg_value);

	return count;
}

static DEVICE_ATTR(reg_addr, 0644, reg_addr_show, reg_addr_store);
static DEVICE_ATTR(reg_value, 0644, reg_value_show, reg_value_store);

static struct attribute *mi_i2c_attrs[] = {
	&dev_attr_reg_addr.attr,
	&dev_attr_reg_value.attr,
	NULL,
};

static const struct attribute_group mi_i2c_attrs_group = {
	.attrs = mi_i2c_attrs,
};

int mi_i2c_sysfs_init(struct i2c_client *client)
{
    int ret;
	ret = sysfs_create_group(&client->dev.kobj, &mi_i2c_attrs_group);
	if (ret) {
		dev_err(&client->dev, "Failed to create sysfs attributes\n");
	}
    return ret;
}
EXPORT_SYMBOL(mi_i2c_sysfs_init);

void mi_i2c_sysfs_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &mi_i2c_attrs_group);
}
EXPORT_SYMBOL(mi_i2c_sysfs_remove);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("A simple example Linux i2c module.");
MODULE_VERSION("0.01");