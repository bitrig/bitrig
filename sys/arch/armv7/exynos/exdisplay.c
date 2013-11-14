/* $OpenBSD: omdog.c,v 1.5 2011/11/15 23:01:11 drahn Exp $ */
/*
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
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <armv7/armv7/armv7var.h>

/* registers */

struct exdisplay_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct exdisplay_softc *exdisplay_sc;

void exdisplay_attach(struct device *parent, struct device *self, void *args);
int exdisplay_cnattach(bus_space_tag_t iot, bus_addr_t iobase, size_t size);
void exdisplay_setup_rasops(struct rasops_info *rinfo, struct wsscreen_descr *descr);

struct cfattach	exdisplay_ca = {
	sizeof (struct exdisplay_softc), NULL, exdisplay_attach
};

struct cfdriver exdisplay_cd = {
	NULL, "exdisplay", DV_DULL
};

bus_space_tag_t		exdisplayiot;
bus_space_handle_t	exdisplayioh;
bus_addr_t		exdisplayaddr;
struct wsscreen_descr	descr;
struct rasops_info	ri;

void
exdisplay_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct exdisplay_softc *sc = (struct exdisplay_softc *) self;

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("exdisplay_attach: bus_space_map failed!");

	printf("\n");
	exdisplay_sc = sc;
}

int
exdisplay_cnattach(bus_space_tag_t iot, bus_addr_t iobase, size_t size)
{
	long defattr;

	if (bus_space_map(iot, iobase, size, 0, &exdisplayioh))
		return ENOMEM;

	exdisplayiot = iot;
	exdisplayaddr = iobase;

	ri.ri_bits = (u_char*)exdisplayioh;

	exdisplay_setup_rasops(&ri, &descr);

	/* assumes 16 bpp */
	ri.ri_ops.alloc_attr(&ri, 0, 0, 0, &defattr);

	wsdisplay_cnattach(&descr, &ri, ri.ri_ccol, ri.ri_crow, defattr);

	return 0;
}

void
exdisplay_setup_rasops(struct rasops_info *rinfo, struct wsscreen_descr *descr)
{
	rinfo->ri_flg = RI_CLEAR;
	rinfo->ri_depth = 16;
	rinfo->ri_width = 1366;
	rinfo->ri_height = 768;
	rinfo->ri_stride = rinfo->ri_width * rinfo->ri_depth / 8;

	/* swap B and R */
	if (rinfo->ri_depth == 16) {
		rinfo->ri_rnum = 5;
		rinfo->ri_rpos = 11;
		rinfo->ri_gnum = 6;
		rinfo->ri_gpos = 5;
		rinfo->ri_bnum = 5;
		rinfo->ri_bpos = 0;
	}

	wsfont_init();
	rinfo->ri_wsfcookie = wsfont_find(NULL, 8, 0, 0);
	wsfont_lock(rinfo->ri_wsfcookie, &rinfo->ri_font,
	    WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R);

	/* get rasops to compute screen size the first time */
	rasops_init(rinfo, 200, 200);

	descr->nrows = rinfo->ri_rows;
	descr->ncols = rinfo->ri_cols;
	descr->capabilities = rinfo->ri_caps;
	descr->textops = &rinfo->ri_ops;
}
