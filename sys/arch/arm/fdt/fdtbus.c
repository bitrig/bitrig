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

#include <arm/fdt/fdtbusvar.h>

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
static void fdt_attach_node(struct device *, void *, void *);
static void fdt_attach_interrupt_controller(struct device *);
static int fdt_attach_clocks(struct device *, void *);

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

static struct device *
fdt_is_attached(void *node)
{
	struct fdt_entry *fe;

	SLIST_FOREACH(fe, &fdt_dev_list, fe_list)
		if (fe->fe_node == node)
			return fe->fe_dev;

	return NULL;
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

	/* Scan for interrupt controllers and attach them first. */
	fdt_attach_interrupt_controller(self);

	/* Scan the rest of the tree. */
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
		if (!fdt_is_attached(node))
			fdt_attach_node(self, match, node);

		fdt_iterate(self, match, fdt_child_node(node));
	}
}

static inline void
fdt_dev_list_insert(void *node, struct device *child)
{
	struct fdt_entry *fe;

	if (child == NULL)
		return;

	fe = malloc(sizeof(*fe), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (fe == NULL)
		panic("%s: cannot allocate memory", __func__);

	fe->fe_node = node;
	fe->fe_dev = child;
	SLIST_INSERT_HEAD(&fdt_dev_list, fe, fe_list);
}

/*
 * Try to attach a node to a known driver.
 */
static void
fdt_attach_node(struct device *self, void *match, void *node)
{
	struct armv7_attach_args aa;
	struct cfdata		*cf = match;
	struct device		*child;
	char			*status;

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

	if (fdt_attach_clocks(self, node)) {
		printf("%s: can't supply needed clocks\n", __func__);
		return;
	}

	child = config_attach(self, cf, &aa, NULL);
	fdt_dev_list_insert(node, child);
}

/*
 * Look for a driver that can handle this node.
 */
static void *
fdt_found_node(struct device *self, void *node)
{
	struct armv7_attach_args aa;
	struct device		*child;
	char			*status;

	if (!fdt_node_property(node, "compatible", NULL))
		return NULL;

	if (fdt_node_property(node, "status", &status))
		if (!strcmp(status, "disabled"))
			return NULL;

	memset(&aa, 0, sizeof(aa));
	aa.aa_dev = NULL;
	aa.aa_iot = &armv7_bs_tag;
	aa.aa_dmat = &fdt_bus_dma_tag;
	aa.aa_node = node;

	child = config_found(self, &aa, NULL);
	fdt_dev_list_insert(node, child);
	return child;
}

struct ic_entry {
	SLIST_ENTRY(ic_entry)	 ie_list;
	void			*ie_node;
	void			*ie_parent;
};

/*
 * Attach interrupt controller before attaching devices. This makes
 * sure we have a controller available to establish interrupts on.
 */
static void
fdt_attach_interrupt_controller(struct device *self)
{
	SLIST_HEAD(, ic_entry) ic_list = SLIST_HEAD_INITIALIZER(ic_list);
	SLIST_HEAD(, ic_entry) ip_list = SLIST_HEAD_INITIALIZER(ip_list);
	struct ic_entry *ie, *tmp;
	void *node = NULL, *parent;

	/* Create a list of all interrupt controllers. */
	while ((node = fdt_find_node_with_prop(fdt_next_node(node),
	    "interrupt-controller")) != NULL) {
		ie = malloc(sizeof(*ie), M_DEVBUF, M_NOWAIT|M_ZERO);
		if (ie == NULL)
			panic("%s: cannot allocate memory", __func__);

		ie->ie_node = node;
		ie->ie_parent = fdt_get_interrupt_controller(node);
		if (ie->ie_parent == node)
			ie->ie_parent = NULL;

		SLIST_INSERT_HEAD(&ic_list, ie, ie_list);
	}

	/* Look for the root interrupt controller first. */
	parent = NULL;

	/* As long as we have unconfigured interrupt controllers. */
	while (!SLIST_EMPTY(&ic_list)) {
		SLIST_FOREACH_SAFE(ie, &ic_list, ie_list, tmp) {
			if (ie->ie_parent != parent)
				continue;

			SLIST_REMOVE(&ic_list, ie, ic_entry, ie_list);

			if (fdt_found_node(self, ie->ie_node) == NULL) {
				free(ie, M_DEVBUF, sizeof(*ie));
				continue;
			}

			SLIST_INSERT_HEAD(&ip_list, ie, ie_list);
		}

		/*
		 * After attaching an interrupt controller, we must
		 * find him here.  If we do not find one, we still
		 * have children left but no parents to attach.
		 */
		ie = SLIST_FIRST(&ip_list);
		if (ie == SLIST_END(&ip_list)) {
			printf("%s: no parents left, but children remaining\n",
			    __func__);
			break;
		}
		parent = ie->ie_node;
		SLIST_REMOVE(&ip_list, ie, ic_entry, ie_list);
		free(ie, M_DEVBUF, sizeof(*ie));
	}

	/* Clean up. */
	SLIST_FOREACH_SAFE(ie, &ip_list, ie_list, tmp) {
		SLIST_REMOVE(&ip_list, ie, ic_entry, ie_list);
		free(ie, M_DEVBUF, sizeof(*ie));
	}
	SLIST_FOREACH_SAFE(ie, &ic_list, ie_list, tmp) {
		SLIST_REMOVE(&ic_list, ie, ic_entry, ie_list);
		free(ie, M_DEVBUF, sizeof(*ie));
	}
}

/*
 * Before attaching a device we need to make sure that all
 * clock controllers the device depends on are attached.
 */
static int
fdt_attach_clocks(struct device *self, void *node)
{
	int idx, nclk;

	nclk = fdt_node_property(node, "clocks", NULL) / sizeof(uint32_t);
	if (nclk <= 0)
		return 0;

	int clocks[nclk];
	if (fdt_node_property_ints(node, "clocks", clocks, nclk) != nclk)
		panic("%s: can't extract clocks, but they exist", __func__);

	for (idx = 0; idx < nclk; idx++) {
		void *clkc = fdt_find_node_by_phandle(NULL, clocks[idx]);
		if (clkc == NULL) {
			printf("%s: can't find clock controller\n",
			    __func__);
			return -1;
		}

		int cells;
		if (!fdt_node_property_int(clkc, "#clock-cells", &cells)) {
			printf("%s: can't find size of clock cells\n",
			    __func__);
			return -1;
		}
		idx += cells;

		if (fdt_is_attached(clkc))
			continue;

		if (fdt_attach_clocks(self, clkc)) {
			printf("%s: can't attach parent clocks\n", __func__);
			return -1;
		}

		if (fdt_found_node(self, clkc) == NULL) {
			printf("%s: can't find clock driver\n", __func__);
			return -1;
		}
	}

	return 0;
}

/*
 * Public API
 */
struct device *
fdt_get_device(void *node)
{
	return fdt_is_attached(node);
}
