
#include <linux/regmap.h>


struct mi_i2c_device {
	struct device *dev;
	uint8_t reg_addr;
	uint8_t reg_value;
	struct regmap *regmap;
};