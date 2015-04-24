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

struct clkff_softc {
	struct device	sc_dev;
	struct clk 	*sc_clk;
};

static int clk_fdt_fixed_factor_match(struct device *, void *, void *);
static void clk_fdt_fixed_factor_attach(struct device *, struct device *, void *);
static struct clk *clk_fdt_fixed_factor_provider_get(void *, void *, int *, int);

struct cfdriver clkff_cd = {
	NULL, "clkff", DV_DULL
};

struct cfattach clkff_ca = {
	sizeof (struct clkff_softc),
	clk_fdt_fixed_factor_match,
	clk_fdt_fixed_factor_attach
};

static int
clk_fdt_fixed_factor_match(struct device *parent, void *self, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fixed-factor-clock", aa->aa_node))
		return 1;

	return 0;
}

static void
clk_fdt_fixed_factor_attach(struct device *parent, struct device *self, void *args)
{
	struct clkff_softc *sc = (struct clkff_softc *)self;
	struct armv7_attach_args *aa = args;
	char *names = fdt_node_name(aa->aa_node);
	uint32_t mult, div;
	struct clk *p;

	printf("\n");

	if (!fdt_node_property_int(aa->aa_node, "clock-div", &div))
		return;

	if (!fdt_node_property_int(aa->aa_node, "clock-mult", &mult))
		return;

	fdt_node_property(aa->aa_node, "clock-output-names", &names);

	p = clk_fdt_get(aa->aa_node, 0);
	if (p == NULL)
		return;

	sc->sc_clk = clk_fixed_factor(names, p, mult, div);

	clk_fdt_register_provider(aa->aa_node,
	    clk_fdt_fixed_factor_provider_get, self);
}

static struct clk *
clk_fdt_fixed_factor_provider_get(void *self, void *node, int *args, int narg)
{
	struct clkff_softc *sc = (struct clkff_softc *)self;
	return sc->sc_clk;
}
