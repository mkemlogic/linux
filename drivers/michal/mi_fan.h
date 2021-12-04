#include <linux/pwm.h>

struct mi_fan_device_priv {
	struct platform_device *pdev;
	struct pwm_device *pwm;
	uint8_t gpio;		
};