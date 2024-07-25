/*-
 * Copyright (c) 2024 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/thread.h>
#include <sys/spinlock.h>

#include <arm/stm/stm32g0.h>

#include <dev/i2c/i2c.h>

extern struct stm32f4_gpio_softc gpio_sc;

extern struct mdx_device dev_i2c1;

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
	ret = stm32f4_i2c_xfer(&dev_i2c1, msgs, 1);
	if (ret != 0) {
		printf("%s: could not configure mcp3421, slave %x\n",
		    __func__, slave);
		return (ret);
	}

	printf("cfg written\n");

	return (0);
}

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
