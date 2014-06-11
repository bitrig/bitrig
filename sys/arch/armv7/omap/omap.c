/* $OpenBSD: omap.c,v 1.4 2013/11/06 19:03:07 syl Exp $ */
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
omap_match(struct device *parent, void *cfdata, void *aux)
{
	switch (board_id)
	{
	case BOARD_ID_OMAP3_BEAGLE:
	case BOARD_ID_AM335X_BEAGLEBONE:
	case BOARD_ID_OMAP3_OVERO:
	case BOARD_ID_OMAP4_PANDA:
		return (1);
	}

	return (0);
}

struct cfattach omap_ca = {
	sizeof(struct armv7_softc), omap_match, armv7_attach
};

struct cfdriver omap_cd = {
	NULL, "omap", DV_DULL
};

struct board_dev beagleboard_devs[] = {
	{ "prcm",	0 },
	{ "intc",	0 },
	{ "gptimer",	0 },
	{ "gptimer",	1 },
	{ "omdog",	0 },
	{ "omgpio",	0 },
	{ "omgpio",	1 },
	{ "omgpio",	2 },
	{ "omgpio",	3 },
	{ "omgpio",	4 },
	{ "omgpio",	5 },
	{ "ommmc",	0 },		/* HSMMC1 */
	{ "com",	2 },		/* UART3 */
	{ NULL,		0 }
};

struct board_dev beaglebone_devs[] = {
	{ "prcm",	0 },
	{ "sitaracm",	0 },
	{ "intc",	0 },
	{ "edma",	0 },
	{ "dmtimer",	0 },
	{ "dmtimer",	1 },
	{ "omdog",	0 },
	{ "omgpio",	0 },
	{ "omgpio",	1 },
	{ "omgpio",	2 },
	{ "omgpio",	3 },
	{ "ommmc",	0 },		/* HSMMC0 */
	{ "ommmc",	1 },		/* HSMMC1 */
	{ "com",	0 },		/* UART0 */
	{ "cpsw",	0 },
	{ NULL,		0 }
};

struct board_dev overo_devs[] = {
	{ "prcm",	0 },
	{ "intc",	0 },
	{ "gptimer",	0 },
	{ "gptimer",	1 },
	{ "omdog",	0 },
	{ "omgpio",	0 },
	{ "omgpio",	1 },
	{ "omgpio",	2 },
	{ "omgpio",	3 },
	{ "omgpio",	4 },
	{ "omgpio",	5 },
	{ "ommmc",	0 },		/* HSMMC1 */
	{ "com",	2 },		/* UART3 */
	{ NULL,		0 }
};

struct board_dev pandaboard_devs[] = {
	{ "omapid",	0 },
	{ "prcm",	0 },
	{ "omdog",	0 },
	{ "omgpio",	0 },
	{ "omgpio",	1 },
	{ "omgpio",	2 },
	{ "omgpio",	3 },
	{ "omgpio",	4 },
	{ "omgpio",	5 },
	{ "ommmc",	0 },		/* HSMMC1 */
	{ "com",	2 },		/* UART3 */
	{ "ehci",	0 },
	{ NULL,		0 }
};
