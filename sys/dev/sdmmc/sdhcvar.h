/*	$OpenBSD: sdhcvar.h,v 1.6 2011/07/31 16:55:01 kettenis Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
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

#ifndef _SDHCVAR_H_
#define _SDHCVAR_H_

#include <machine/bus.h>

struct sdhc_host;

struct sdhc_softc {
	struct device		sc_dev;
	struct sdhc_host	**sc_host;
	int			sc_nhosts;
	uint32_t		sc_flags;
	uint32_t		sc_caps;
	uint32_t		sc_clkbase;
	int			sc_clkmsk;	/* Mask for SDCLK */

	int (*sc_vendor_rod)(struct sdhc_softc *, int);
	int (*sc_vendor_card_detect)(struct sdhc_softc *);
	int (*sc_vendor_bus_clock)(struct sdhc_softc *, int);
};

/* Host controller functions called by the attachment driver. */
int	sdhc_host_found(struct sdhc_softc *, bus_space_tag_t,
	    bus_space_handle_t, bus_size_t);
int	sdhc_activate(struct device *, int);
void	sdhc_shutdown(void *);
int	sdhc_intr(void *);

/* flag values */
#define	SDHC_F_USE_DMA		0x0001
#define	SDHC_F_FORCE_DMA	0x0002
#define	SDHC_F_NOPWR0		0x0004
#define	SDHC_F_32BIT_ACCESS	0x0010
#define	SDHC_F_ENHANCED		0x0020	/* Freescale ESDHC */
#define	SDHC_F_8BIT_MODE	0x0040	/* MMC 8bit mode is supported */
#define	SDHC_F_HOSTCAPS		0x0200	/* No device provided capabilities */
#define	SDHC_F_NO_HS_BIT	0x2000	/* Don't set SDHC_HIGH_SPEED bit */

#endif
