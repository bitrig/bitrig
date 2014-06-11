/* $OpenBSD: imx.c,v 1.3 2013/11/06 19:03:07 syl Exp $ */
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

#include <armv7/armv7/armv7var.h>

static int
imx_match(struct device *parent, void *cfdata, void *aux)
{
	switch (board_id)
	{
	case BOARD_ID_IMX6_HUMMINGBOARD:
	case BOARD_ID_IMX6_SABRELITE:
	case BOARD_ID_IMX6_UDOO:
	case BOARD_ID_IMX6_UTILITE:
	case BOARD_ID_IMX6_WANDBOARD:
		return (1);
	}

	return (0);
}

struct cfattach imx_ca = {
	sizeof(struct armv7_softc), imx_match, armv7_attach, NULL,
	config_activate_children
};

struct cfdriver imx_cd = {
	NULL, "imx", DV_DULL
};

struct board_dev hummingboard_devs[] = {
	{ "imxocotp",	0 },
	{ "imxccm",	0 },
	{ "imxtemp",	0 },
	{ "imxiomuxc",	0 },
	{ "imxdog",	0 },
	{ "imxuart",	0 },
	{ "imxgpio",	0 },
	{ "imxgpio",	1 },
	{ "imxgpio",	2 },
	{ "imxgpio",	3 },
	{ "imxgpio",	4 },
	{ "imxgpio",	5 },
	{ "imxgpio",	6 },
	{ "imxesdhc",	1 },
	{ "ehci",	0 },
	{ "ehci",	1 },
	{ "imxenet",	0 },
	{ NULL,		0 }
};

struct board_dev sabrelite_devs[] = {
	{ "imxocotp",	0 },
	{ "imxccm",	0 },
	{ "imxtemp",	0 },
	{ "imxiomuxc",	0 },
	{ "imxdog",	0 },
	{ "imxuart",	1 },
	{ "imxgpio",	0 },
	{ "imxgpio",	1 },
	{ "imxgpio",	2 },
	{ "imxgpio",	3 },
	{ "imxgpio",	4 },
	{ "imxgpio",	5 },
	{ "imxgpio",	6 },
	{ "imxesdhc",	2 },
	{ "imxesdhc",	3 },
	{ "ehci",	0 },
	{ "imxenet",	0 },
	{ "ahci",	0 },
	{ NULL,		0 }
};

struct board_dev udoo_devs[] = {
	{ "imxocotp",	0 },
	{ "imxccm",	0 },
	{ "imxtemp",	0 },
	{ "imxiomuxc",	0 },
	{ "imxdog",	0 },
	{ "imxuart",	1 },
	{ "imxgpio",	0 },
	{ "imxgpio",	1 },
	{ "imxgpio",	2 },
	{ "imxgpio",	3 },
	{ "imxgpio",	4 },
	{ "imxgpio",	5 },
	{ "imxgpio",	6 },
	{ "imxesdhc",	2 },
	{ "imxesdhc",	3 },
	{ "ehci",	0 },
	{ "imxenet",	0 },
	{ "ahci",	0 },
	{ NULL,		0 }
};

struct board_dev utilite_devs[] = {
	{ "imxocotp",	0 },
	{ "imxccm",	0 },
	{ "imxtemp",	0 },
	{ "imxiomuxc",	0 },
	{ "imxdog",	0 },
	{ "imxuart",	3 },
	{ "imxgpio",	0 },
	{ "imxgpio",	1 },
	{ "imxgpio",	2 },
	{ "imxgpio",	3 },
	{ "imxgpio",	4 },
	{ "imxgpio",	5 },
	{ "imxgpio",	6 },
	{ "imxiic",	2 },
	{ "imxesdhc",	2 },
	{ "ehci",	0 },
	{ "imxenet",	0 },
	{ "ahci",	0 },
	{ NULL,		0 }
};

struct board_dev wandboard_devs[] = {
	{ "imxocotp",	0 },
	{ "imxccm",	0 },
	{ "imxtemp",	0 },
	{ "imxiomuxc",	0 },
	{ "imxdog",	0 },
	{ "imxuart",	0 },
	{ "imxgpio",	0 },
	{ "imxgpio",	1 },
	{ "imxgpio",	2 },
	{ "imxgpio",	3 },
	{ "imxgpio",	4 },
	{ "imxgpio",	5 },
	{ "imxgpio",	6 },
	{ "imxenet",	0 },
	{ "imxesdhc",	2 },
	{ "imxesdhc",	0 },
	{ "ehci",	0 },
	{ "ahci",	0 },	/* only on quad, afaik. */
	{ NULL,		0 }
};
