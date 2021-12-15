#include <linux/pwm.h>
#include <linux/gpio/consumer.h>

struct mi_fan_pwm_device_priv {
	struct platform_device *pdev;
	struct pwm_device *pwm;
	struct gpio_desc *gpio_desc;
};