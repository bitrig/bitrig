/* $OpenBSD: exynos.c,v 1.8 2015/07/17 17:33:50 jsg Exp $ */
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

#include <machine/bus.h>
#include <machine/fdt.h>

#include <armv7/armv7/armv7var.h>

static int
exynos_match(struct device *parent, void *cfdata, void *aux)
{
	/* If we're running with fdt, do not attach. */
	/* XXX: Find a better way. */
	if (fdt_next_node(0))
		return (0);

	switch (board_id)
	{
	case BOARD_ID_EXYNOS5_CHROMEBOOK:
		return (1);
	}

	return (0);
}

struct cfattach exynos_ca = {
	sizeof(struct armv7_softc), exynos_match, armv7_attach, NULL,
	config_activate_children
};

struct cfdriver exynos_cd = {
	NULL, "exynos", DV_DULL
};

struct board_dev chromebook_devs[] = {
	{ "exmct",	0 },
	{ "exdog",	0 },
	{ "exclock",	0 },
	{ "expower",	0 },
	{ "exsysreg",	0 },
//	{ "exuart",	1 },
	{ "exgpio",	0 },
	{ "exgpio",	1 },
	{ "exgpio",	2 },
	{ "exgpio",	3 },
	{ "exgpio",	4 },
	{ "exgpio",	5 },
	{ "exehci",	0 },
	{ "exiic",	4 },
//	{ "exesdhc",	2 },
//	{ "exesdhc",	3 },
	{ "exdisplay",	0 },
	{ NULL,		0 }
};
