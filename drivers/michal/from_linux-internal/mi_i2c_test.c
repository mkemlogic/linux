#include <linux/module.h>
#include <linux/i2c.h>


int mi_i2c_transfer_test(struct i2c_client *client)
{
	int ret;
	char buf[5];
	unsigned char start = 0x00; /* start reading at this address*/

		struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &start,
		}, {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 5,
			.buf	= buf,
		}
	};

	ret = i2c_transfer(client->adapter, &msgs[0], 2);

	if(ret != 2)
		dev_err(&client->dev, "%s: Failed to transfer data", __func__);
	else
		dev_info(&client->dev, "%s: Received data: 0x%02x, 0x%02x,0x%02x, 0x%02x, 0x%02x",
			__func__, buf[0], buf[1], buf[2], buf[3], buf[4]);
	return (ret != 2);
}

EXPORT_SYMBOL(mi_i2c_transfer_test);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Koziel");
MODULE_DESCRIPTION("A simple example Linux i2c module.");
MODULE_VERSION("0.01");