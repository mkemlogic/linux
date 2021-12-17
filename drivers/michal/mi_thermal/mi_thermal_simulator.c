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

static int mi_thermal_get_temp(void *drv_data, int *temp)
{
	struct mi_thermal_priv *priv = container_of(drv_data, struct mi_thermal_priv, pdev);

	*temp = priv->temp;

	return 0;
}

#if (IS_ENABLED(CONFIG_THERMAL_EMULATION))
static int mi_thermal_set_emul_temp(void *drv_data, int temp)
{
	struct mi_thermal_priv *priv = container_of(drv_data, struct mi_thermal_priv, pdev);

	priv->temp = temp;

	return 0;
}
#endif

static const struct thermal_zone_of_device_ops mi_thermal_of_ops = {
	.get_temp	= mi_thermal_get_temp,
	.set_emul_temp = (IS_ENABLED(CONFIG_THERMAL_EMULATION)) ? mi_thermal_set_emul_temp : NULL,
};

#if (!IS_ENABLED(CONFIG_THERMAL_EMULATION))
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

	return count;
}
static DEVICE_ATTR(temp, 0644, NULL, temp_store);

static struct attribute *mi_thermal_attrs[] = {
	&dev_attr_temp.attr,
	NULL,
};

ATTRIBUTE_GROUPS(mi_thermal);
#endif /* (!IS_ENABLED(CONFIG_THERMAL_EMULATION)) */

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
	priv->temp = 29000;

	/* no_hwmon:
	 *	a boolean to indicate if the thermal to hwmon sysfs interface
	 *	is required. when no_hwmon == false, a hwmon sysfs interface
	 *	will be created. when no_hwmon == true, nothing will be done.
	 *	In case the thermal_zone_params is NULL, the hwmon interface
	 *	will be created (for backward compatibility).
	*/
	thermal->tzp->no_hwmon = false;

	ret = thermal_add_hwmon_sysfs(thermal);
	if (ret)
		return ret;

#if (!IS_ENABLED(CONFIG_THERMAL_EMULATION))
	ret = sysfs_create_groups(&pdev->dev.kobj, mi_thermal_groups);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}
#endif

	return 0;
}

static int mi_thermal_remove(struct platform_device *pdev)
{
	struct mi_thermal_priv *priv = platform_get_drvdata(pdev);

	thermal_remove_hwmon_sysfs(priv->thermal);
#if (!IS_ENABLED(CONFIG_THERMAL_EMULATION))
	sysfs_remove_group(&pdev->dev.kobj, &mi_thermal_group);
#endif

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
