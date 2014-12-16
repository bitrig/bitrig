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
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/fdt.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxocotpvar.h>

/* registers */
#define OCOTP_MAC0	0x620
#define OCOTP_MAC1	0x630
#define OCOTP_ANA0	0x4d0
#define OCOTP_ANA1	0x4e0
#define OCOTP_ANA2	0x4f0

struct imxocotp_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct imxocotp_softc *imxocotp_sc;

int imxocotp_match(struct device *parent, void *v, void *aux);
void imxocotp_attach(struct device *parent, struct device *self, void *args);

struct cfattach imxocotp_ca = {
	sizeof (struct imxocotp_softc), NULL, imxocotp_attach
};
struct cfattach imxocotp_fdt_ca = {
	sizeof (struct imxocotp_softc), imxocotp_match, imxocotp_attach
};

struct cfdriver imxocotp_cd = {
	NULL, "imxocotp", DV_DULL
};

int
imxocotp_match(struct device *parent, void *v, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fsl,imx6q-ocotp", aa->aa_node))
		return 1;

	return 0;
}

void
imxocotp_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct imxocotp_softc *sc = (struct imxocotp_softc *) self;
	struct fdt_memory mem;

	sc->sc_iot = aa->aa_iot;
	if (aa->aa_node) {
		if (fdt_get_memory_address(aa->aa_node, 0, &mem))
			panic("%s: could not extract memory data from FDT",
			    __func__);
	} else {
		mem.addr = aa->aa_dev->mem[0].addr;
		mem.size = aa->aa_dev->mem[0].size;
	}

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");
	imxocotp_sc = sc;
}

int
imxocotp_get_ethernet_address(u_int8_t *mac)
{
	uint32_t value;

	value = bus_space_read_4(imxocotp_sc->sc_iot, imxocotp_sc->sc_ioh, OCOTP_MAC0);
	if (!value)
		return 1;

	mac[5] = value & 0xff;
	mac[4] = (value >> 8) & 0xff;
	mac[3] = (value >> 16) & 0xff;
	mac[2] = (value >> 24) & 0xff;

	value = bus_space_read_4(imxocotp_sc->sc_iot, imxocotp_sc->sc_ioh, OCOTP_MAC1);
	mac[1] = value & 0xff;
	mac[0] = (value >> 8) & 0xff;

	return 0;
}

uint32_t
imxocotp_get_temperature_calibration(void)
{
	return bus_space_read_4(imxocotp_sc->sc_iot, imxocotp_sc->sc_ioh, OCOTP_ANA1);
}
