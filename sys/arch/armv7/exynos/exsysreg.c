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
#include <armv7/exynos/exvar.h>
#include <armv7/exynos/exsysregvar.h>

/* registers */
#define SYSREG_USB20PHY_CFG			0x230

/* bits and bytes */
#define SYSREG_USB20PHY_CFG_HOST_LINK_EN	(1 << 0)

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct exsysreg_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct exsysreg_softc *exsysreg_sc;

void exsysreg_attach(struct device *parent, struct device *self, void *args);

struct cfattach	exsysreg_ca = {
	sizeof (struct exsysreg_softc), NULL, exsysreg_attach
};

struct cfdriver exsysreg_cd = {
	NULL, "exsysreg", DV_DULL
};

void
exsysreg_attach(struct device *parent, struct device *self, void *args)
{
	struct ex_attach_args *ea = args;
	struct exsysreg_softc *sc = (struct exsysreg_softc *) self;

	exsysreg_sc = sc;
	sc->sc_iot = ea->ea_iot;
	if (bus_space_map(sc->sc_iot, ea->ea_dev->mem[0].addr,
	    ea->ea_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("exsysreg_attach: bus_space_map failed!");

	printf("\n");
}

void
exsysreg_usbhost_mode(int on)
{
	struct exsysreg_softc *sc = exsysreg_sc;

	if (on)
		HSET4(sc, SYSREG_USB20PHY_CFG, SYSREG_USB20PHY_CFG_HOST_LINK_EN);
	else
		HCLR4(sc, SYSREG_USB20PHY_CFG, SYSREG_USB20PHY_CFG_HOST_LINK_EN);
}
