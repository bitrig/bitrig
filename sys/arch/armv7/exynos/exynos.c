/* $OpenBSD: omap.c,v 1.3 2011/11/15 23:01:11 drahn Exp $ */
/*
 * Copyright (c) 2005,2008 Dale Rahn <drahn@openbsd.com>
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
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/reboot.h>
#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <arch/arm/armv7/armv7var.h>
#include <armv7/exynos/exvar.h>

struct arm32_bus_dma_tag exynos_bus_dma_tag = {
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

struct board_dev {
	char *name;
	int unit;
};

struct board_dev chromebook_devs[] = {
	{ "exmct",	0 },
	{ "exdog",	0 },
	{ "exclock",	0 },
	{ "expower",	0 },
	{ "exsysreg",	0 },
//	{ "exiomuxc",	0 },
//	{ "exuart",	1 },
	{ "exgpio",	0 },
	{ "exgpio",	1 },
	{ "exgpio",	2 },
	{ "exgpio",	3 },
	{ "exgpio",	4 },
	{ "ehci",	0 },
	{ "exiic",	4 },
//	{ "exesdhc",	2 },
//	{ "exesdhc",	3 },
	{ NULL,		0 }
};

struct board_dev *board_devs;

struct ex_dev *ex_devs = NULL;

struct exynos_softc {
	struct device sc_dv;
};

int	exynos_match(struct device *, void *, void *);
void	exynos_attach(struct device *, struct device *, void *);
int	exynos_submatch(struct device *, void *, void *);

struct cfattach exynos_ca = {
	sizeof(struct exynos_softc), exynos_match, exynos_attach, NULL,
	config_activate_children
};

struct cfdriver exynos_cd = {
	NULL, "exynos", DV_DULL
};

int
exynos_match(struct device *parent, void *cfdata, void *aux)
{
	return (1);
}

void
exynos_attach(struct device *parent, struct device *self, void *aux)
{
	struct board_dev *bd;

	switch (board_id) {
	case BOARD_ID_EXYNOS5_CHROMEBOOK:
		printf(": Exynos 5 Chromebook\n");
		exynos5_init();
		board_devs = chromebook_devs;
		break;
	default:
		printf("\n");
		panic("%s: board type 0x%x unknown", __func__, board_id);
	}

	/* Directly configure on-board devices (dev* in config file). */
	for (bd = board_devs; bd->name != NULL; bd++) {
		struct ex_dev *id = ex_find_dev(bd->name, bd->unit);
		struct ex_attach_args ea;

		if (id == NULL)
			printf("%s: device %s unit %d not found\n",
			    self->dv_xname, bd->name, bd->unit);

		memset(&ea, 0, sizeof(ea));
		ea.ea_dev = id;
		ea.ea_iot = &armv7_bs_tag;
		ea.ea_dmat = &exynos_bus_dma_tag;

		if (config_found_sm(self, &ea, NULL, exynos_submatch) == NULL)
			printf("%s: device %s unit %d not configured\n",
			    self->dv_xname, bd->name, bd->unit);
	}
}

/*
 * We do direct configuration of devices on this SoC "bus", so we
 * never call the child device's match function at all (it can be
 * NULL in the struct cfattach).
 */
int
exynos_submatch(struct device *parent, void *child, void *aux)
{
	struct cfdata *cf = child;
	struct ex_attach_args *ea = aux;

	if (strcmp(cf->cf_driver->cd_name, ea->ea_dev->name) == 0)
		return (1);

	/* "These are not the droids you are looking for." */
	return (0);
}

void
ex_set_devs(struct ex_dev *devs)
{
	ex_devs = devs;
}

struct ex_dev *
ex_find_dev(const char *name, int unit)
{
	struct ex_dev *id;

	if (ex_devs == NULL)
		panic("%s: ex_devs == NULL", __func__);

	for (id = ex_devs; id->name != NULL; id++) {
		if (id->unit == unit && strcmp(id->name, name) == 0)
			return (id);
	}

	return (NULL);
}
