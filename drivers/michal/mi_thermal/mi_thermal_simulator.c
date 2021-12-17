// SPDX-License-Identifier: GPL-2.0+
/*
 * Broadcom AVS RO thermal sensor driver
 *
 * based on brcmstb_thermal
 *
 * Copyright (C) 2020 Stefan Wahren
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/thermal.h>

#include "../../thermal/thermal_hwmon.h"

struct mi_thermal_priv {
	struct platform_device *pdev;
	struct thermal_zone_device *thermal;
	int temp;
};

int mi_temperature = 30000;

static int mi_thermal_get_temp(void *data, int *temp)
{
	*temp = mi_temperature;
	return 0;
}

static const struct thermal_zone_of_device_ops mi_thermal_of_ops = {
	.get_temp	= mi_thermal_get_temp,
};


static ssize_t temp_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_thermal_priv *priv = platform_get_drvdata(pdev);
	unsigned long temp;
	int ret;

	ret = kstrtol(buf, 10, &temp);
	if (ret < 0)
		return count;

	priv->temp = temp;
	mi_temperature = temp;

	return count;
}
static DEVICE_ATTR(temp, 0644, NULL, temp_store);

static struct attribute *mi_thermal_attrs[] = {
	&dev_attr_temp.attr,
	NULL,
};

ATTRIBUTE_GROUPS(mi_thermal);


static int mi_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal;
	struct mi_thermal_priv *priv;
	struct device *dev = &pdev->dev;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;


	thermal = devm_thermal_zone_of_sensor_register(dev, 0, priv,
						       &mi_thermal_of_ops);
	if (IS_ERR(thermal)) {
		ret = PTR_ERR(thermal);
		dev_err(dev, "could not register sensor: %d\n", ret);
		return ret;
	}

	priv->thermal = thermal;

	thermal->tzp->no_hwmon = false;
	ret = thermal_add_hwmon_sysfs(thermal);
	if (ret)
		return ret;

	ret = sysfs_create_groups(&pdev->dev.kobj, mi_thermal_groups);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}

	return 0;
}

static int mi_thermal_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &mi_thermal_group);

    return 0;
}


static const struct of_device_id mi_thermal_id_table[] = {
	{ .compatible = "mi,mi-thermal-simulator" },
	{},
};
MODULE_DEVICE_TABLE(of, mi_thermal_id_table);

static struct platform_driver mi_thermal_driver = {
	.probe = mi_thermal_probe,
	.remove = mi_thermal_remove,
	.driver = {
		.name = "mi_thermal",
		.of_match_table = mi_thermal_id_table,
	},
};
module_platform_driver(mi_thermal_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefan Wahren");
MODULE_DESCRIPTION("Broadcom AVS RO thermal sensor driver");
