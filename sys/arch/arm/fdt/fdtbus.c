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
#include <sys/malloc.h>
#include <sys/queue.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/fdt.h>

#include <arm/armv7/armv7var.h>

/* FIXME: use own attach args */
#include <armv7/armv7/armv7var.h>

struct arm32_bus_dma_tag fdt_bus_dma_tag = {
	0,
	0,
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

static int fdt_match(struct device *, void *, void *);
static void fdt_attach(struct device *, struct device *, void *);
static void fdt_find_match(struct device *, void *);
static void fdt_iterate(struct device *, struct device *, void *);
static void fdt_try_node(struct device *, void *, void *);

/*
 * We need to look through the tree at least twice.  To make sure we
 * do not attach node multiple times, keep a list of nodes that are
 * already handled by any kind of driver.
 */
SLIST_HEAD(, fdt_entry) fdt_dev_list = SLIST_HEAD_INITIALIZER(fdt_dev_list);
struct fdt_entry {
	SLIST_ENTRY(fdt_entry)	 fe_list;
	void			*fe_node;
	struct device		*fe_dev;
};

static int
fdt_is_attached(void *node)
{
	struct fdt_entry *fe;

	SLIST_FOREACH(fe, &fdt_dev_list, fe_list)
		if (fe->fe_node == node)
			return (1);

	return (0);
}

struct cfattach fdt_ca = {
	sizeof(struct armv7_softc), fdt_match, fdt_attach, NULL,
	config_activate_children
};

struct cfdriver fdt_cd = {
	NULL, "fdt", DV_DULL
};

/*
 * FDT bus always attaches when there is some kind of FDT available.
 */
static int
fdt_match(struct device *parent, void *cfdata, void *aux)
{
	if (fdt_next_node(0) != NULL)
		return (1);

	return (0);
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

	config_scan(fdt_find_match, self);
}

/*
 * Usually you should be able to attach devices in the order
 * specified in the device tree.  Due to how a few drivers
 * are written we need to make sure we attach them in the
 * order stated in files.xxx.
 *
 * This means for every driver enables, we go through the
 * FDT and look for compatible nodes.
 */
static void
fdt_find_match(struct device *self, void *match)
{
	fdt_iterate(self, match, fdt_next_node(0));
}

static void
fdt_iterate(struct device *self, struct device *match, void *node)
{
	for (;
	    node != NULL;
	    node = fdt_next_node(node))
	{
		/* skip nodes that are already handled by some driver */
		if (fdt_is_attached(node))
			continue;

		fdt_iterate(self, match, fdt_child_node(node));
		fdt_try_node(self, match, node);
	}
}

static void
fdt_try_node(struct device *self, void *match, void *node)
{
	struct cfdata *cf = match;
	struct armv7_attach_args aa;
	struct device *child;
	char  *status;

	if (!fdt_node_property(node, "compatible", NULL))
		return;

	if (fdt_node_property(node, "status", &status))
		if (!strcmp(status, "disabled"))
			return;

	memset(&aa, 0, sizeof(aa));
	aa.aa_dev = NULL;
	aa.aa_iot = &armv7_bs_tag;
	aa.aa_dmat = &fdt_bus_dma_tag;
	aa.aa_node = node;

	/* allow for devices to be disabled in UKC */
	if ((*cf->cf_attach->ca_match)(self, cf, &aa) == 0)
		return;

	child = config_attach(self, cf, &aa, NULL);
	if (child != NULL) {
		struct fdt_entry *fe = malloc(sizeof(*fe), M_DEVBUF,
		    M_NOWAIT|M_ZERO);
		fe->fe_node = node;
		fe->fe_dev = child;
		SLIST_INSERT_HEAD(&fdt_dev_list, fe, fe_list);
	}
}
