/* $OpenBSD: sunxi.c,v 1.3 2013/11/06 19:03:07 syl Exp $ */
/*
 * Copyright (c) 2005,2008 Dale Rahn <drahn@openbsd.com>
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
sunxi_match(struct device *parent, void *cfdata, void *aux)
{
	switch (board_id)
	{
	case BOARD_ID_SUN4I_A10:
	case BOARD_ID_SUN7I_A20:
		return (1);
	}

	return (0);
}

struct cfattach sunxi_ca = {
	sizeof(struct armv7_softc), sunxi_match, armv7_attach
};

struct cfdriver sunxi_cd = {
	NULL, "sunxi", DV_DULL
};

struct board_dev sun4i_devs[] = {
	{ "sxipio",	0 },
	{ "sxiccmu",	0 },
	{ "a1xintc",	0 },
	{ "sxitimer",	0 },
	{ "sxidog",	0 },
	{ "sxirtc",	0 },
	{ "sxiuart",	0 },
	{ "sxiuart",	1 },
	{ "sxiuart",	2 },
	{ "sxiuart",	3 },
	{ "sxiuart",	4 },
	{ "sxiuart",	5 },
	{ "sxiuart",	6 },
	{ "sxiuart",	7 },
	{ "sxie",	0 },
	{ "ahci",	0 },
	{ "ehci",	0 },
	{ "ehci",	1 },
#if 0
	{ "ohci",	0 },
	{ "ohci",	1 },
#endif
	{ NULL,		0 }
};

struct board_dev sun7i_devs[] = {
	{ "sxipio",	0 },
	{ "sxiccmu",	0 },
	{ "sxitimer",	0 },
	{ "sxidog",	0 },
	{ "sxirtc",	0 },
	{ "sxiuart",	0 },
	{ "sxiuart",	1 },
	{ "sxiuart",	2 },
	{ "sxiuart",	3 },
	{ "sxiuart",	4 },
	{ "sxiuart",	5 },
	{ "sxiuart",	6 },
	{ "sxiuart",	7 },
	{ "sxie",	0 },
	{ "ahci",	0 },
	{ "ehci",	0 },
	{ "ehci",	1 },
#if 0
	{ "ohci",	0 },
	{ "ohci",	1 },
#endif
	{ NULL,		0 }
};
