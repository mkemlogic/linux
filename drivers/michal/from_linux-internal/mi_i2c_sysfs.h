#include <linux/i2c.h>

int mi_i2c_sysfs_init(struct i2c_client *client);
void mi_i2c_sysfs_remove(struct i2c_client *client);