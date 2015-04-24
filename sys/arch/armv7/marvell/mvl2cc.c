/*
 * Copyright (c) 2015 Patrick Wildt <patrick@blueri.se>
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
#include <machine/fdt.h>
#include <arm/cpufunc.h>
#include <armv7/armv7/armv7var.h>

/* registers */
#define L2C_CTL				0x100
#define L2C_AUXCTL			0x104
#define L2C_CNTR__CTL			0x200
#define L2C_CNTR_CONF(x)		(0x204 + (x) * 0xc)
#define L2C_INT_CAUSE			0x220
#define L2C_CFU				0x228
#define L2C_SYNC			0x700
#define L2C_STATUS			0x704
#define L2C_RANGE_BASE			0x720
#define L2C_INV_PHYS			0x770
#define L2C_INV_RANGE			0x774
#define L2C_INV_IDXWAY			0x778
#define L2C_INV_WAY			0x77c
#define L2C_BLOCK			0x78c
#define L2C_WB_PHYS			0x7b0
#define L2C_WB_RANGE			0x7b4
#define L2C_WB_IDXWAY			0x7b8
#define L2C_WB_WAY			0x7bc
#define L2C_WBINV_PHYS			0x7f0
#define L2C_WBINV_RANGE			0x7f4
#define L2C_WBINV_IDXWAY		0x7f8
#define L2C_WBINV_WAY			0x7fc
#define L2C_ENABLE			(1 << 0)
#define L2C_WBWT_MODE_MASK		(3 << 0)
#define L2C_REP_STRAT_MASK		(3 << 27)
#define L2C_REP_STRAT_SEMIPLRU		(3 << 27)
#define L2C_ALL_WAYS			0xffffffff

#define roundup2(size, unit) (((size) + (unit) - 1) & ~((unit) - 1))

struct mvl2cc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	uint32_t		sc_enabled;
};

struct mvl2cc_softc *mvl2cc_sc;

int mvl2cc_match(struct device *, void *, void *);
void mvl2cc_attach(struct device *parent, struct device *self, void *args);
void mvl2cc_enable(struct mvl2cc_softc *);
void mvl2cc_sdcache_wbinv_all(void);
void mvl2cc_sdcache_wbinv_range(vaddr_t, paddr_t, psize_t);
void mvl2cc_sdcache_inv_range(vaddr_t, paddr_t, psize_t);
void mvl2cc_sdcache_wb_range(vaddr_t, paddr_t, psize_t);
void mvl2cc_cache_sync(struct mvl2cc_softc *);
void mvl2cc_sdcache_drain_writebuf(void);

struct cfattach mvliicc_ca = {
	sizeof (struct mvl2cc_softc), mvl2cc_match, mvl2cc_attach
};

struct cfdriver mvliicc_cd = {
	NULL, "mvliicc", DV_DULL
};

int
mvl2cc_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,aurora-system-cache", aa->aa_node))
		return (1);

	return (0);
}

void
mvl2cc_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct mvl2cc_softc *sc = (struct mvl2cc_softc *) self;
	struct fdt_memory mem;

	printf("\n");

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: cannot extract memory", sc->sc_dev.dv_xname);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", sc->sc_dev.dv_xname);

	mvl2cc_sc = sc;

	if (bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CTL))
		panic("L2 Cache controller was already enabled");

	mvl2cc_enable(sc);
	sc->sc_enabled = 1;

	mvl2cc_sdcache_wbinv_all();

	cpufuncs.cf_sdcache_wbinv_all = mvl2cc_sdcache_wbinv_all;
	cpufuncs.cf_sdcache_wbinv_range = mvl2cc_sdcache_wbinv_range;
	cpufuncs.cf_sdcache_inv_range = mvl2cc_sdcache_inv_range;
	cpufuncs.cf_sdcache_wb_range = mvl2cc_sdcache_wb_range;
	cpufuncs.cf_sdcache_drain_writebuf = mvl2cc_sdcache_drain_writebuf;
}

void
mvl2cc_enable(struct mvl2cc_softc *sc)
{
	u_int32_t auxctl, reg;
	int s;
	s = splhigh();

	/* Set L2 policy */
	auxctl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_AUXCTL);
	auxctl &= ~L2C_WBWT_MODE_MASK;
	auxctl &= ~L2C_REP_STRAT_MASK;
	auxctl |= L2C_REP_STRAT_SEMIPLRU;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_AUXCTL, auxctl);

	/* Invalidate L2 cache */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_WBINV_WAY, L2C_ALL_WAYS);

	/* Clear pending L2 interrupts */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_INT_CAUSE, 0x1ff);

	/* Enable Cache and TLB maintenance broadcast */
	__asm volatile ("mrc p15, 1, %0, c15, c2, 0" : "=r"(reg));
	reg |= (1 << 8);
	__asm volatile ("mcr p15, 1, %0, c15, c2, 0" : :"r"(reg));

	/* Set the Point of Coherency and Point of Unification to DRAM. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CFU,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CFU) |
	    ((1 << 17) | (1 << 18)));

	/* Enable L2 cache */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_CTL,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, L2C_CTL) | L2C_ENABLE);

	mvl2cc_cache_sync(sc);

	splx(s);
}

void
mvl2cc_cache_sync(struct mvl2cc_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_SYNC, 0);
}

void
mvl2cc_sdcache_drain_writebuf(void)
{
	struct mvl2cc_softc *sc = mvl2cc_sc;
	if (sc == NULL || !sc->sc_enabled)
		return;

	mvl2cc_cache_sync(sc);
}

void
mvl2cc_sdcache_wbinv_all(void)
{
	struct mvl2cc_softc *sc = mvl2cc_sc;
	if (sc == NULL || !sc->sc_enabled)
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_WBINV_WAY, L2C_ALL_WAYS);
	mvl2cc_cache_sync(sc);
}
void
mvl2cc_sdcache_wbinv_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct mvl2cc_softc *sc = mvl2cc_sc;
	if (sc == NULL || !sc->sc_enabled)
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_RANGE_BASE,
	    pa & ~0x1f);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_WBINV_RANGE,
	    ((pa & ~0x1f) + len + 0x20) & ~0x1f);
	mvl2cc_cache_sync(sc);
	__asm volatile ("dsb");
}

void
mvl2cc_sdcache_inv_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct mvl2cc_softc *sc = mvl2cc_sc;
	if (sc == NULL || !sc->sc_enabled)
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_RANGE_BASE,
	    pa & ~0x1f);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_INV_RANGE,
	    ((pa & ~0x1f) + len + 0x20) & ~0x1f);
}

void
mvl2cc_sdcache_wb_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct mvl2cc_softc *sc = mvl2cc_sc;
	if (sc == NULL || !sc->sc_enabled)
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_RANGE_BASE,
	    pa & ~0x1f);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, L2C_WB_RANGE,
	    ((pa & ~0x1f) + len + 0x20) & ~0x1f);
	mvl2cc_cache_sync(sc);
	__asm volatile("dsb");
}
