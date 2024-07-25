#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/thread.h>
#include <sys/spinlock.h>

#include <arm/stm/stm32g0.h>

#include <dev/i2c/i2c.h>
//#include <dev/i2c/bitbang/i2c_bitbang.h>

extern struct stm32f4_gpio_softc gpio_sc;

extern struct mdx_device dev_i2c1;

//static struct i2c_bitbang_softc i2c_bitbang_sc;
//static struct mdx_device dev_bitbang = { .sc = &i2c_bitbang_sc };

static void
gpio_set(int port, int pin, int val)
{

	pin_set(&gpio_sc, port, pin, val);
}

static int
mcp3421_get_mv(uint8_t unit)
{
	struct i2c_msg msgs[1];
	uint8_t data[3];
	int b0, b1, b2;
	int mv;
	int ret;

	msgs[0].slave = 0x68 << 1;

	bzero(&data, 3);

	msgs[0].buf = data;
	msgs[0].len = 3;
	msgs[0].flags = IIC_M_RD;

	//ret = mdx_i2c_transfer(&dev_bitbang, msgs, 1);
	ret = stm32f4_i2c_xfer(&dev_i2c1, msgs, 1);

	if (ret == 0) {
		b2 = data[0];
		b1 = data[1];
		b0 = data[2];

		printf("%s(%d): read %d %d %02x\n", __func__, unit, b2, b1, b0);
		mv = b2 << 8 | b1;
		return (mv * 1);
        }

        return (-1);
}

static int
mcp3421_configure(uint8_t slave)
{
	struct i2c_msg msgs[1];
	uint8_t cfg;
	int ret;

	/* Configure the MCP3421. */
	cfg = 0x10 | (1 << 2);
	msgs[0].slave = slave;
	msgs[0].buf = &cfg;
	msgs[0].len = 1;
	msgs[0].flags = IIC_M_NOSTOP;
	//ret = mdx_i2c_transfer(&dev_bitbang, msgs, 1);
	ret = stm32f4_i2c_xfer(&dev_i2c1, msgs, 1);
	if (ret != 0) {
		printf("%s: could not configure mcp3421, slave %x\n",
		    __func__, slave);
		return (ret);
	}

	printf("cfg written\n");

	return (0);
}

#if 0
static const struct gpio_pin uart_pins[] = {
	{ PORT_A,  9, MODE_OUT, 0, PULLDOWN }, /* SCL */
	{ PORT_A, 10, MODE_INP, 0, PULLDOWN }, /* SDA */
	{ -1, -1, -1, -1, -1 },
};
#endif

#if 0
static void
i2c_sda(void *arg, bool enable)
{

	const struct gpio_pin pins_inp[] = {
		{ PORT_A, 10, MODE_INP, 0, FLOAT }, /* SDA */
		{ -1, -1, -1, -1, -1 },
	};

	const struct gpio_pin pins_out[] = {
		{ PORT_A, 10, MODE_OUT, 0, FLOAT }, /* SDA */
		{ -1, -1, -1, -1, -1 },
	};

	if (enable) {
		pin_configure(&gpio_sc, pins_inp);
	} else {
		pin_configure(&gpio_sc, pins_out);
	}
}

static void
i2c_scl(void *arg, bool enable)
{

	const struct gpio_pin pins_inp[] = {
		{ PORT_A, 9, MODE_INP, 0, FLOAT }, /* SCL */
		{ -1, -1, -1, -1, -1 },
	};

	const struct gpio_pin pins_out[] = {
		{ PORT_A, 9, MODE_OUT, 0, FLOAT }, /* SCL */
		{ -1, -1, -1, -1, -1 },
	};

	if (enable)
		pin_configure(&gpio_sc, pins_inp);
        else
		pin_configure(&gpio_sc, pins_out);
}

static int
i2c_sda_val(void *arg)
{

	if (pin_get(&gpio_sc, PORT_A, 10))
		return (1);

	return (0);
}

static struct i2c_bitbang_ops i2c_ops = {
	.i2c_scl = &i2c_scl,
	.i2c_sda = &i2c_sda,
	.i2c_sda_val = &i2c_sda_val,
};
#endif

int
main(void)
{
	int mv;

	printf("hello world\n");

	//i2c_bitbang_init(&dev_bitbang, &i2c_ops);

	printf("sldr started\n");

	gpio_set(PORT_A, 6, 0); /* HEATER_EN */
	gpio_set(PORT_C, 6, 1); /* LED disable */

	//mcp3421_configure(0x68);
	//while (1) {
		mcp3421_configure(0x68 << 1);
		mdx_usleep(500000);
	//}

	while (1) {
		mv = mcp3421_get_mv(0);
		printf("mv %d\n", mv);

		if (mv >= 0 && mv < 4) {
			gpio_set(PORT_A, 6, 1); /* HEATER_EN */
			gpio_set(PORT_C, 6, 0); /* LED disable */
			mdx_usleep(500000);
			gpio_set(PORT_A, 6, 0); /* HEATER_EN */
			gpio_set(PORT_C, 6, 1); /* LED disable */
		}

		mdx_usleep(500000);
	}

	return (0);
}
