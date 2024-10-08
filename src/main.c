/*-
 * Copyright (c) 2024 Ruslan Bukin <br@bsdpad.com>
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
extern struct stm32g0_exti_softc exti_sc;
extern struct mdx_device dev_i2c1;

static int global_enable;

#define	dprintf(fmt, ...)

static void
gpio_set(int port, int pin, int val)
{

	pin_set(&gpio_sc, port, pin, val);
}

static void
exti_handler(void *arg, int raising)
{

	dprintf("%s: %d\n", __func__, raising);

	if (raising) {
		if (global_enable == 0)
			global_enable = 1;
		else
			global_enable = 0;
	}

	if (global_enable)
		gpio_set(PORT_C, 6, 0); /* LED */
	else
		gpio_set(PORT_C, 6, 1); /* LED */
}

struct exti_intr_entry map[] = {
	{ },
	{ },
	{ },
	{ },
	{ },
	{ },
	{ },
	{ .handler = exti_handler, .arg = NULL },
};

static int
mcp3421_get_mv(uint8_t addr, uint32_t *mv)
{
	struct i2c_msg msgs[1];
	uint8_t data[3];
	int b0 __unused, b1, b2;
	int ret;

	msgs[0].slave = addr;
	msgs[0].buf = data;
	msgs[0].len = 3;
	msgs[0].flags = IIC_M_RD;

	ret = stm32f4_i2c_xfer(&dev_i2c1, msgs, 1);
	if (ret == 0) {
		b2 = data[0];
		b1 = data[1];
		b0 = data[2];

		dprintf("%s: read %d %d %02x\n", __func__, b2, b1, b0);
		*mv = b2 << 8 | b1;
		return (0);
        }

        return (-1);
}

static int
mcp3421_configure(uint8_t addr)
{
	struct i2c_msg msgs[1];
	uint8_t cfg;
	int ret;

	/* Configure the mcp3421. */

	cfg = 0x10 | (1 << 2);

	msgs[0].slave = addr;
	msgs[0].buf = &cfg;
	msgs[0].len = 1;
	msgs[0].flags = 0;

	ret = stm32f4_i2c_xfer(&dev_i2c1, msgs, 1);
	if (ret != 0) {
		printf("%s: could not configure mcp3421, addr %x\n",
		    __func__, addr);
		return (ret);
	}

	printf("cfg written\n");

	return (0);
}

static int
get_delay_us(int mv)
{
	uint32_t val;

	if (mv < 10)
		val = 500000;
	else if (mv < 15)
		val = 300000;
	else if (mv < 17)
		val = 150000;
	else
		val = 100000;

        return (val);
}

int
main(void)
{
	uint32_t us;
	uint32_t mv;
	uint8_t addr;
	int error;

	printf("sldr started\n");

	/* Starting disabled. */
	global_enable = 0;
	gpio_set(PORT_A, 6, 0); /* Heater disable */
	gpio_set(PORT_C, 6, 1); /* LED disable */

	addr = (0x68 << 1);

	stm32g0_exti_install_intr_map(&exti_sc, map);

	mcp3421_configure(addr);

	while (1) {
		mdx_usleep(50000);

		error = mcp3421_get_mv(addr, &mv);
		if (error) {
			printf("%s: error reading mcp3421\n", __func__);
			continue;
		}

		printf("%s: enable %d mv %d\n", __func__, global_enable, mv);

		if (global_enable) {
			if (mv >= 0 && mv < 19) {
				us = get_delay_us(mv);

				gpio_set(PORT_A, 6, 1); /* Heater enable. */
				mdx_usleep(us);
				gpio_set(PORT_A, 6, 0); /* Heater disable. */
			}
		} else
			mdx_usleep(500000);
	}

	return (0);
}
