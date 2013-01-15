/* $OpenBSD: omdog.c,v 1.5 2011/11/15 23:01:11 drahn Exp $ */
/*
 * Copyright (c) 2013 Patrick Wildt <webmaster@patrick-wildt.de>
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
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/cpufunc.h>
#include <imx/dev/imxvar.h>

#define PL310_ERRATA_727915

/* registers */
#define L2C_CACHE_ID			0x000
#define L2C_CACHE_TYPE			0x004
#define L2C_CTL				0x100
#define L2C_AUXCTL			0x104
#define L2C_TAG_RAM_CTL			0x108
#define L2C_DATA_RAM_CTL		0x10c
#define L2C_EVC_CTR_CTL			0x200
#define L2C_EVC_CTR0_CTL		0x204
#define L2C_EVC_CTR1_CTL		0x208
#define L2C_EVC_CTR0_VAL		0x20c
#define L2C_EVC_CTR1_VAL		0x210
#define L2C_INT_MASK			0x214
#define L2C_INT_MASK_STS		0x218
#define L2C_INT_RAW_STS			0x21c
#define L2C_INT_CLR			0x220
#define L2C_CACHE_SYNC			0x730
#define L2C_INV_PA			0x770
#define L2C_INV_WAY			0x77c
#define L2C_CLEAN_PA			0x7b0
#define L2C_CLEAN_INDEX			0x7b8
#define L2C_CLEAN_WAY			0x7bc
#define L2C_CLEAN_INV_PA		0x7f0
#define L2C_CLEAN_INV_INDEX		0x7f8
#define L2C_CLEAN_INV_WAY		0x7fc
#define L2C_D_LOCKDOWN0			0x900
#define L2C_I_LOCKDOWN0			0x904
#define L2C_D_LOCKDOWN1			0x908
#define L2C_I_LOCKDOWN1			0x90c
#define L2C_D_LOCKDOWN2			0x910
#define L2C_I_LOCKDOWN2			0x914
#define L2C_D_LOCKDOWN3			0x918
#define L2C_I_LOCKDOWN3			0x91c
#define L2C_D_LOCKDOWN4			0x920
#define L2C_I_LOCKDOWN4			0x924
#define L2C_D_LOCKDOWN5			0x928
#define L2C_I_LOCKDOWN5			0x92c
#define L2C_D_LOCKDOWN6			0x930
#define L2C_I_LOCKDOWN6			0x934
#define L2C_D_LOCKDOWN7			0x938
#define L2C_I_LOCKDOWN7			0x93c
#define L2C_LOCKDOWN_LINE_EN		0x950
#define L2C_UNLOCK_WAY			0x954
#define L2C_ADDR_FILTER_START		0xc00
#define L2C_ADDR_FILTER_END		0xc04
#define L2C_DEBUG_CTL			0xf40
#define L2C_PREFETCH_CTL		0xf60
#define L2C_POWER_CTL			0xf80

#define L2C_CACHE_ID_RELEASE_MASK	0x3f
#define L2C_CACHE_TYPE_LINESIZE		0x3
#define L2C_AUXCTL_ASSOC_SHIFT		16
#define L2C_AUXCTL_ASSOC_MASK		0x1

#define roundup2(size, unit) (((size) + (unit) - 1) & ~((unit) - 1))

struct imxlii_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	uint32_t		sc_enabled;
	uint32_t		sc_waymask;
	uint32_t		sc_dcache_line_size;
};

struct imxlii_softc *imxlii_sc;

void imxlii_attach(struct device *parent, struct device *self, void *args);
void imxlii_enable(struct imxlii_softc *);
void imxlii_disable(struct imxlii_softc *);
void imxlii_sdcache_wbinv_all(void);
void imxlii_sdcache_wbinv_range(vaddr_t, paddr_t, psize_t);
void imxlii_sdcache_inv_range(vaddr_t, paddr_t, psize_t);
void imxlii_sdcache_wb_range(vaddr_t, paddr_t, psize_t);
void imxlii_cache_range_op(paddr_t, psize_t, bus_size_t);
void imxlii_cache_way_op(struct imxlii_softc *, bus_size_t, uint32_t);
void imxlii_cache_op(struct imxlii_softc *, bus_size_t, uint32_t);
void imxlii_cache_sync(struct imxlii_softc *);

struct cfattach imxlii_ca = {
	sizeof (struct imxlii_softc), NULL, imxlii_attach
};

struct cfdriver imxlii_cd = {
	NULL, "imxlii", DV_DULL
};

void
imxlii_attach(struct device *parent, struct device *self, void *args)
{
	struct imx_attach_args *ia = args;
	struct imxlii_softc *sc = (struct imxlii_softc *) self;

	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_dev->mem[0].addr,
	    ia->ia_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("imxlii_attach: bus_space_map failed!");

	printf(": rtl %d", bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CACHE_ID) & 0x3f);
	printf("\n");

	imxlii_sc = sc;

	if (bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CTL))
		panic("L2 Cache controller was already enabled\n");

	sc->sc_dcache_line_size = 32 << (bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CACHE_TYPE) & L2C_CACHE_TYPE_LINESIZE);
	sc->sc_waymask = (8 << ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_AUXCTL)
			    >> L2C_AUXCTL_ASSOC_SHIFT) & L2C_AUXCTL_ASSOC_MASK)) - 1;
	printf("waymask: 0x%08x\n", sc->sc_waymask);

	imxlii_enable(sc);
	sc->sc_enabled = 1;

	imxlii_sdcache_wbinv_all();

	cpufuncs.cf_sdcache_wbinv_all = imxlii_sdcache_wbinv_all;
	cpufuncs.cf_sdcache_wbinv_range = imxlii_sdcache_wbinv_range;
	cpufuncs.cf_sdcache_inv_range = imxlii_sdcache_inv_range;
	cpufuncs.cf_sdcache_wb_range = imxlii_sdcache_wb_range;

	return;
}

void
imxlii_enable(struct imxlii_softc *sc)
{
	int s;
	s = splhigh();

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CTL, 1);

	imxlii_cache_way_op(sc, L2C_INV_WAY, sc->sc_waymask);
	imxlii_cache_sync(sc);

	splx(s);
}

void
imxlii_disable(struct imxlii_softc *sc)
{
	int s;
	s = splhigh();

	imxlii_cache_way_op(sc, L2C_CLEAN_INV_WAY, sc->sc_waymask);
	imxlii_cache_sync(sc);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CTL, 0);

	splx(s);
}

void
imxlii_cache_op(struct imxlii_softc *sc, bus_size_t off, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, val);
	while (bus_space_read_4(sc->sc_iot, sc->sc_ioh, off) & 1) {
		/* spin */
	}
}

void
imxlii_cache_way_op(struct imxlii_softc *sc, bus_size_t off, uint32_t way_mask)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, off, way_mask);
	while (bus_space_read_4(sc->sc_iot, sc->sc_ioh, off) & way_mask) {
		/* spin */
	}
}

void
imxlii_cache_sync(struct imxlii_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CACHE_SYNC, 0xffffffff);
}

void
imxlii_cache_range_op(paddr_t pa, psize_t len, bus_size_t cache_op)
{
	struct imxlii_softc * const sc = imxlii_sc;
	int s;
	size_t line_size = sc->sc_dcache_line_size;
	size_t line_mask = line_size - 1;
	paddr_t endpa;
	paddr_t segend;
	size_t off = pa & line_mask;
	if (off) {
		len += off;
		pa -= off;
	}
	len = roundup2(len, line_size);
	off = pa & PAGE_MASK;
	for (endpa = pa + len; pa < endpa; off = 0) {
		psize_t seglen = min(len, PAGE_SIZE - off);

		s = splhigh();
		if (!sc->sc_enabled) {
			splx(s);
			return;
		}
		for (segend = pa + seglen; pa < segend; pa += line_size) {
			imxlii_cache_op(sc, cache_op, pa);
		}
		splx(s);
	}
}

void
imxlii_sdcache_wbinv_all(void)
{
	struct imxlii_softc *sc = imxlii_sc;
	if ((sc == NULL) || !sc->sc_enabled)
		return;

#ifdef PL310_ERRATA_727915
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_DEBUG_CTL, 3);
#endif

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CLEAN_INV_WAY, sc->sc_waymask);
	while(bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CLEAN_INV_WAY) & sc->sc_waymask);

#ifdef PL310_ERRATA_727915
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_DEBUG_CTL, 0);
#endif

	imxlii_cache_sync(sc);
}
void
imxlii_sdcache_wbinv_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct imxlii_softc *sc = imxlii_sc;
	if ((sc == NULL) || !sc->sc_enabled)
		return;

#ifdef PL310_ERRATA_727915
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_DEBUG_CTL, 3);
#endif

	imxlii_cache_range_op(pa, len, L2C_CLEAN_INV_PA);
	imxlii_cache_sync(sc);

#ifdef PL310_ERRATA_727915
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_DEBUG_CTL, 0);
#endif
}

void
imxlii_sdcache_inv_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct imxlii_softc *sc = imxlii_sc;
	if ((sc == NULL) || !sc->sc_enabled)
		return;
	imxlii_cache_range_op(pa, len, L2C_INV_PA);
	imxlii_cache_sync(sc);
}

void
imxlii_sdcache_wb_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct imxlii_softc *sc = imxlii_sc;
	if ((sc == NULL) || !sc->sc_enabled)
		return;
	imxlii_cache_range_op(pa, len, L2C_CLEAN_PA);
	imxlii_cache_sync(sc);
}
