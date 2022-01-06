/* Input device simulator
 *
 * - use mi_event in sysfs to simulate event
 *
 * Device tree bindings
 *
  	mi_input_simulator: mi-input-simulator {
			compatible = "mi,mi-thermal-simulator";
	};
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



struct mi_input_priv {
	struct platform_device *pdev;
	struct input_dev *input_dev;
};

static ssize_t mi_event_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mi_input_priv *priv = platform_get_drvdata(pdev);
	int ret;
	int temp;

	ret = kstrtol(buf, 10, &temp);
	if (ret < 0)
		return count;

	return count;
}
static DEVICE_ATTR(mi_event, 0644, NULL, mi_event_store);

static struct attribute *mi_input_attrs[] = {
	&dev_attr_mi_event.attr,
	NULL,
};

ATTRIBUTE_GROUPS(mi_input);


static int mi_input_probe(struct platform_device *pdev)
{
	struct mi_input_priv *priv;
	struct device *dev = &pdev->dev;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;

	priv->input_dev = input_allocate_device();
	if (!priv->input_dev) {
		printk(KERN_ERR "button.c: Not enough memory\n");
		return -ENOMEM;
    }


	ret = thermal_add_hwmon_sysfs(thermal);
	if (ret)
		return ret;

	ret = sysfs_create_groups(&pdev->dev.kobj, mi_input_groups);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}

	return 0;
}

static int mi_thermal_remove(struct platform_device *pdev)
{
	struct mi_thermal_priv *priv = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &mi_input_groups);

	return 0;
}


static const struct of_device_id mi_thermal_id_table[] = {
	{ .compatible = "mi,mi-input-simulator" },
	{},
};
MODULE_DEVICE_TABLE(of, mi_thermal_id_table);

static struct platform_driver mi_thermal_driver = {
	.probe = mi_input_probe,
	.remove = mi_thermal_remove,
	.driver = {
		.name = "mi_input_simulator",
		.of_match_table = mi_thermal_id_table,
	},
};
module_platform_driver(mi_thermal_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("Temperature sensor emulator");
