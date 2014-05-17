/*	$OpenBSD: pca9532.c,v 1.3 2008/04/17 16:50:17 deraadt Exp $ */
/*
 * Copyright (c) 2006 Dale Rahn <drahn@openbsd.org>
 * Copyright (c) 2013 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/gpio.h>

#include <dev/i2c/i2cvar.h>
#include <dev/gpio/gpiovar.h>

/* driver for MAX 7301 */

#define MAX7301_ADDR 0x60

#define MAX7301_GPIO_PIN_CMD_BASE	0x20
#define MAX7301_GPIO_PIN_OFFSET		4	/* first four pins are unused */
#define MAX7301_GPIO_PIN_CTL_CMD_BASE	0x09
#define MAX7301_GPIO_NPINS		28
#define MAX7301_GPIO_NPIN_CTL		7

struct max7301_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_addr;
	struct gpio_chipset_tag sc_gpio_gc;
	struct gpio_pin sc_gpio_pin[MAX7301_GPIO_NPINS];

};

int max7301_match(struct device *, void *, void *);
void max7301_attach(struct device *, struct device *, void *);
int max7301_gpio_pin_read(void *arg, int pin);
void max7301_gpio_pin_write (void *arg, int pin, int value);
void max7301_gpio_pin_ctl (void *arg, int pin, int flags);

struct cfattach maxgpio_ca = {
	sizeof(struct max7301_softc), max7301_match, max7301_attach
};

struct cfdriver maxgpio_cd = {
	NULL, "max7301", DV_DULL
};

int
max7301_match(struct device *parent, void *v, void *arg)
{
	struct i2c_attach_args *ia = arg;
	int ok = 0;
	uint8_t cmd, data;

	if (ia->ia_addr != MAX7301_ADDR)
		return (0);
	/* attempt to read input register 0 */
	iic_acquire_bus(ia->ia_tag, I2C_F_POLL);
	cmd = 0;
	if (iic_exec(ia->ia_tag, I2C_OP_READ_WITH_STOP, ia->ia_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail;
	ok = 1;
fail:
	iic_release_bus(ia->ia_tag, I2C_F_POLL);
	return (ok);
}

void
max7301_attach(struct device *parent, struct device *self, void *arg)
{
	struct max7301_softc *sc = (void *)self;
	struct i2c_attach_args *ia = arg;
	struct gpiobus_attach_args gba;
	int i;
	uint8_t cmd, data;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);

	/* power up and disable irq output */
	cmd = 4;
	data = 1;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail;

	/* reset it to input */
	for (i = 0; i < MAX7301_GPIO_NPIN_CTL; i++) {
		cmd = MAX7301_GPIO_PIN_CTL_CMD_BASE + i;
		data = 0xAA;

		if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
			goto fail; /* XXX */
	}

	for (i = 0; i < MAX7301_GPIO_NPINS; i++) {
		sc->sc_gpio_pin[i].pin_num = i;
		sc->sc_gpio_pin[i].pin_caps = GPIO_PIN_PULLUP |
		    GPIO_PIN_OUTPUT | GPIO_PIN_INPUT;

		cmd = MAX7301_GPIO_PIN_CMD_BASE + MAX7301_GPIO_PIN_OFFSET + i;

		if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
			goto fail; /* XXX */

		sc->sc_gpio_pin[i].pin_state = 0;
	}
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = max7301_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = max7301_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = max7301_gpio_pin_ctl;

	printf(": MAX7301 GPIO extender\n");

	gba.gba_name = "gpio";
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pin;
	gba.gba_npins = MAX7301_GPIO_NPINS;
#if NGPIO > 0
	config_found(&sc->sc_dev, &gba, gpiobus_print);
#endif

fail:
	iic_release_bus(sc->sc_tag, I2C_F_POLL);
}

int
max7301_gpio_pin_read(void *arg, int pin)
{
	struct max7301_softc *sc = arg;
	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);
	uint8_t cmd, data;

	cmd = MAX7301_GPIO_PIN_CMD_BASE + MAX7301_GPIO_PIN_OFFSET + pin;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail; /* XXX */

fail:
	iic_release_bus(sc->sc_tag, I2C_F_POLL);
	return data & 1;
}

void
max7301_gpio_pin_write(void *arg, int pin, int value)
{
	struct max7301_softc *sc = arg;
	uint8_t cmd, data;

	cmd = MAX7301_GPIO_PIN_CMD_BASE + MAX7301_GPIO_PIN_OFFSET + pin;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail; /* XXX */

	data &= 0x1;
	data |= value;

	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail; /* XXX */

fail:
	iic_release_bus(sc->sc_tag, I2C_F_POLL);
}

void
max7301_gpio_pin_ctl (void *arg, int pin, int flags)
{
	struct max7301_softc *sc = arg;
	uint8_t cmd, data, val = 0;

	cmd = MAX7301_GPIO_PIN_CTL_CMD_BASE + (pin / 4);

	if (flags & GPIO_PIN_INPUT)
		val |= 0x2;
	if (flags & GPIO_PIN_OUTPUT)
		val |= 0x1;
	if (flags & GPIO_PIN_PULLUP)
		val |= 0x1;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail; /* XXX */

	data &= ~(0x3 << (pin % 4));
	data |= (val << (pin % 4));

	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &cmd, sizeof cmd, &data, sizeof data, I2C_F_POLL))
		goto fail; /* XXX */

fail:
	iic_release_bus(sc->sc_tag, I2C_F_POLL);
}

