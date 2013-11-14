/* $OpenBSD: omdog.c,v 1.5 2011/11/15 23:01:11 drahn Exp $ */
/*
 * Copyright (c) 2012-2013 Patrick Wildt <patrick@blueri.se>
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
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/exynos/expowervar.h>

/* registers */
#define POWER_PHY_CTRL				0x708

/* bits and bytes */
#define POWER_PHY_CTRL_USB_HOST_EN		(1 << 0)

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct expower_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct expower_softc *expower_sc;

void expower_attach(struct device *parent, struct device *self, void *args);

struct cfattach	expower_ca = {
	sizeof (struct expower_softc), NULL, expower_attach
};

struct cfdriver expower_cd = {
	NULL, "expower", DV_DULL
};

void
expower_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct expower_softc *sc = (struct expower_softc *) self;

	expower_sc = sc;
	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("expower_attach: bus_space_map failed!");

	printf("\n");
}

void
expower_usbhost_phy_ctrl(int on)
{
	struct expower_softc *sc = expower_sc;

	if (on)
		HSET4(sc, POWER_PHY_CTRL, POWER_PHY_CTRL_USB_HOST_EN);
	else
		HCLR4(sc, POWER_PHY_CTRL, POWER_PHY_CTRL_USB_HOST_EN);
}
