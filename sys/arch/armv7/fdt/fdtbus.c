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

#include <machine/fdt.h>
#include <machine/bus.h>

#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>

extern struct arm32_bus_dma_tag armv7_bus_dma_tag;

static int
fdt_match(struct device *parent, void *cfdata, void *aux)
{
	if (fdt_next_node(0) != NULL)
		return (1);

	return (0);
}

static void
fdt_try_node(struct device *self, void *node)
{
	struct armv7_dev ad;
	struct armv7_attach_args aa;
	char  *status;

	if (!fdt_node_property(node, "compatible", NULL))
		return;

	if (fdt_node_property(node, "status", &status))
		if (!strcmp(status, "disabled"))
			return;

	memset(&ad, 0, sizeof(ad));
	memset(&aa, 0, sizeof(aa));
	aa.aa_dev = &ad;
	aa.aa_iot = &armv7_bs_tag;
	aa.aa_dmat = &armv7_bus_dma_tag;
	aa.aa_node = node;

	config_found(self, &aa, NULL);
}

static void
fdt_iterate(struct device *self, void *node)
{
	for (;
	    node != NULL;
	    node = fdt_next_node(node))
	{
		fdt_iterate(self, fdt_child_node(node));
		fdt_try_node(self, node);
	}
}

static void
fdt_attach(struct device *parent, struct device *self, void *aux)
{
	char *compatible;
	void *node = fdt_next_node(0);
	if (node == NULL) {
		printf(": tree not found\n");
		return;
	}

	if (fdt_node_property(node, "compatible", &compatible))
		printf(": %s compatible\n", compatible);
	else
		printf(": unknown\n");

	fdt_iterate(self, fdt_child_node(node));
}

struct cfattach fdt_ca = {
	sizeof(struct armv7_softc), fdt_match, fdt_attach, NULL,
	config_activate_children
};

struct cfdriver fdt_cd = {
	NULL, "fdt", DV_DULL
};
