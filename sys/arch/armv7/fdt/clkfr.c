/*
 * Copyright (c) 2015 Patrick Wildt <patrick@blueri.se>
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
#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

struct clkfr_softc {
	struct device	sc_dev;
	struct clk 	*sc_clk;
};

static int clk_fdt_fixed_rate_match(struct device *, void *, void *);
static void clk_fdt_fixed_rate_attach(struct device *, struct device *, void *);
static struct clk *clk_fdt_fixed_rate_provider_get(void *, void *, int *, int);

struct cfdriver clkfr_cd = {
	NULL, "clkfr", DV_DULL
};

struct cfattach clkfr_ca = {
	sizeof (struct clkfr_softc),
	clk_fdt_fixed_rate_match,
	clk_fdt_fixed_rate_attach
};

static int
clk_fdt_fixed_rate_match(struct device *parent, void *self, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fixed-clock", aa->aa_node))
		return 1;

	return 0;
}

static void
clk_fdt_fixed_rate_attach(struct device *parent, struct device *self, void *args)
{
	struct clkfr_softc *sc = (struct clkfr_softc *)self;
	struct armv7_attach_args *aa = args;
	char *names = fdt_node_name(aa->aa_node);
	uint32_t freq;

	printf("\n");

	fdt_node_property(aa->aa_node, "clock-output-names", &names);

	if (!fdt_node_property_int(aa->aa_node, "clock-frequency", &freq))
		return;

	sc->sc_clk = clk_fixed_rate(names, freq);

	clk_fdt_register_provider(aa->aa_node,
	    clk_fdt_fixed_rate_provider_get, self);
}

static struct clk *
clk_fdt_fixed_rate_provider_get(void *self, void *node, int *args, int narg)
{
	struct clkfr_softc *sc = (struct clkfr_softc *)self;
	return sc->sc_clk;
}
