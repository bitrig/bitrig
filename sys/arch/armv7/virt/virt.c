/* $OpenBSD: virt.c,v 1.3 2013/11/06 19:03:07 syl Exp $ */
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
virt_match(struct device *parent, void *cfdata, void *aux)
{
	switch (board_id)
	{
	case BOARD_ID_VIRT:
		return (1);
	}

	return (0);
}

static void
virt_attach(struct device *parent, struct device *self, void *aux)
{
	printf(": Virt\n");

	void *node = fdt_next_node(0);
	if (node == NULL)
		return;

	for (node = fdt_child_node(node);
	     node != NULL;
	     node = fdt_next_node(node))
	{
		struct armv7_dev ad;
		struct armv7_attach_args aa;

		if (!fdt_node_property(node, "compatible", NULL))
			continue;

		memset(&ad, 0, sizeof(ad));
		memset(&aa, 0, sizeof(aa));
		aa.aa_dev = &ad;
		aa.aa_iot = &armv7_bs_tag;
		aa.aa_dmat = &armv7_bus_dma_tag;
		aa.aa_node = node;

		config_found(self, &aa, NULL);
	}
}

struct cfattach virt_ca = {
	sizeof(struct armv7_softc), virt_match, virt_attach, NULL,
	config_activate_children
};

struct cfdriver virt_cd = {
	NULL, "virt", DV_DULL
};
