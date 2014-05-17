/*
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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

#include <machine/bus.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/imx/at24var.h>

/* driver for AT24 */

#define AT24_ADDR 0x50
#define AT24_LIMIT 128

struct at24_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_addr;
};

struct at24_softc *at24_softc;

int at24_match(struct device *, void *, void *);
void at24_attach(struct device *, struct device *, void *);
void at24_read(struct at24_softc *, char *, int, int);

struct cfattach atrom_ca = {
	sizeof(struct at24_softc), at24_match, at24_attach
};

struct cfdriver atrom_cd = {
	NULL, "atrom", DV_DULL
};

int
at24_match(struct device *parent, void *v, void *arg)
{
	struct i2c_attach_args *ia = arg;

	if (ia->ia_addr == AT24_ADDR)
		return (1);

	return (0);
}

void
at24_attach(struct device *parent, struct device *self, void *arg)
{
	struct at24_softc *sc = (void *)self;
	struct i2c_attach_args *ia = arg;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	printf("\n");
	at24_softc = sc;
}

void
at24_read(struct at24_softc *sc, char *buf, int offset, int count)
{
	int ret = 0;
	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);

	while (count--) {
		ret = iic_smbus_read_byte(sc->sc_tag, sc->sc_addr, offset++, buf++, I2C_F_POLL);
		if (ret)
			goto fail; /* XXX */
	}

fail:
	iic_release_bus(sc->sc_tag, I2C_F_POLL);
}

int
at24_get_ethernet_address(char *buf)
{
	if (at24_softc != NULL && board_id == BOARD_ID_IMX6_UTILITE) {
		at24_read(at24_softc, buf, 4, 6);
		return 0;
	}
	return ENXIO;
}
