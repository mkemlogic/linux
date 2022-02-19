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
#include <linux/input.h>



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
	long int temp;

	ret = kstrtol(buf, 10, &temp);
	if (ret < 0)
		return count;

	input_report_key(priv->input_dev, BTN_0, temp);
	input_sync(priv->input_dev);


	return count;
}
static DEVICE_ATTR(mi_event, 0644, NULL, mi_event_store);

static struct attribute *mi_input_attrs[] = {
	&dev_attr_mi_event.attr,
	NULL,
};

ATTRIBUTE_GROUPS(mi_input);

static int mi_input_open(struct input_dev *dev){ return 0;}
static void mi_input_close(struct input_dev *dev){}

static int mi_input_probe(struct platform_device *pdev)
{
	struct mi_input_priv *priv;
	struct device *dev = &pdev->dev;
	struct input_dev *input;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;

	input = devm_input_allocate_device(dev);
	if (!input) {
		printk(KERN_ERR "Not enough memory\n");
		return -ENOMEM;
	}

	input->name = "mi_input_simulator";
	input->phys = "mi_input0";


	/* let's make a fake touch panel */
	input->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);

	__set_bit(BTN_TOOL_PEN, input->keybit);


	input_set_abs_params(input, ABS_X, 0, 10, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, 10, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, 10, 0, 0);

	input_set_abs_params(input, ABS_DISTANCE, 0, 10, 0, 0);
	input_set_abs_params(input, ABS_TILT_X, -10, 10, 0, 0);
	input_set_abs_params(input, ABS_TILT_Y, -10, 10, 0, 0);

	input_abs_set_res(input, ABS_X, 1);
	input_abs_set_res(input, ABS_Y, 1);

	priv->input_dev = input;
	ret = input_register_device(input);

	if (ret) {
		printk(KERN_ERR "Failed to register device\n");
		return ret;
	}

	ret = sysfs_create_groups(&pdev->dev.kobj, mi_input_groups);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
	}

	return 0;
}

static int mi_input_remove(struct platform_device *pdev)
{
	struct mi_input_priv *priv = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &mi_input_group);

	return 0;
}


static const struct of_device_id mi_thermal_id_table[] = {
	{ .compatible = "mi,mi-input-simulator" },
	{},
};
MODULE_DEVICE_TABLE(of, mi_thermal_id_table);

static struct platform_driver mi_thermal_driver = {
	.probe = mi_input_probe,
	.remove = mi_input_remove,
	.driver = {
		.name = "mi_input_simulator",
		.of_match_table = mi_thermal_id_table,
	},
};
module_platform_driver(mi_thermal_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("Temperature sensor emulator");
