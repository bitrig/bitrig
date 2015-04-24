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
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/stdint.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

/* registers */
#define TIMER_CTRL		0x0000
#define 	TIMER_WDT_ENABLE	(1 << 8)
#define 	TIMER_WDT_25MHZ_ENABLE	(1 << 10)
#define TIMER_EVENTS_STATUS	0x0004
#define 	TIMER_WDT_EXPIRED	(1U << 31)
#define TIMER_WDT		0x0034

#define WD_RSTOUT		0x0000
#define 	WD_RSTOUT_MASK		(1 << 8)

#define HREAD4(sc, space, reg)						\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ ## space ## _ioh, (reg)))
#define HWRITE4(sc, space, reg, val)					\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ ## space ## _ioh, (reg), (val))
#define HSET4(sc, space, reg, bits)					\
	HWRITE4((sc), space, (reg), HREAD4((sc), space, (reg)) | (bits))
#define HCLR4(sc, space, reg, bits)					\
	HWRITE4((sc), space, (reg), HREAD4((sc), space, (reg)) & ~(bits))

struct mvaxpwdt_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_tmr_ioh;
	bus_space_handle_t	 sc_rst_ioh;
	uint32_t		 sc_clk_rate;
	uint32_t		 sc_max_period;
};

int	 mvaxpwdt_match(struct device *, void *, void *);
void	 mvaxpwdt_attach(struct device *, struct device *, void *);
int	 mvaxpwdt_wdog(void *, int);

struct cfattach	mvaxpwdt_ca = {
	sizeof (struct mvaxpwdt_softc), mvaxpwdt_match, mvaxpwdt_attach
};

struct cfdriver mvaxpwdt_cd = {
	NULL, "mvaxpwdt", DV_DULL
};

int
mvaxpwdt_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-xp-wdt", aa->aa_node))
		return (1);

	return 0;
}

void
mvaxpwdt_attach(struct device *parent, struct device *self, void *args)
{
	struct mvaxpwdt_softc *sc = (struct mvaxpwdt_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	struct clk *clk;

	printf("\n");

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_tmr_ioh))
		panic("%s: bus_space_map failed!", __func__);

	if (fdt_get_memory_address(aa->aa_node, 1, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_rst_ioh))
		panic("%s: bus_space_map failed!", __func__);

	clk = clk_fdt_get_by_name(aa->aa_node, "fixed");
	if (clk == NULL)
		panic("%s: cannot extract clock!", __func__);

	clk_enable(clk);
	sc->sc_clk_rate = clk_get_rate(clk) * 1000;
	sc->sc_max_period = UINT32_MAX / sc->sc_clk_rate;

	/* initialize */
	HSET4(sc, tmr, TIMER_CTRL, TIMER_WDT_25MHZ_ENABLE);
	mvaxpwdt_wdog(self, 0);

	wdog_register(mvaxpwdt_wdog, sc);
}

int
mvaxpwdt_wdog(void *self, int period)
{
	struct mvaxpwdt_softc *sc = (struct mvaxpwdt_softc *)self;

	if (period > sc->sc_max_period)
		period = sc->sc_max_period;

	if (period) { /* enable */
		HWRITE4(sc, tmr, TIMER_WDT, sc->sc_clk_rate * period);
		HCLR4(sc, tmr, TIMER_EVENTS_STATUS, TIMER_WDT_EXPIRED);
		HSET4(sc, tmr, TIMER_CTRL, TIMER_WDT_ENABLE);
		HSET4(sc, rst, WD_RSTOUT, WD_RSTOUT_MASK);
	} else { /* disable */
		HCLR4(sc, rst, WD_RSTOUT, WD_RSTOUT_MASK);
		HCLR4(sc, tmr, TIMER_CTRL, TIMER_WDT_ENABLE);
	}

	return (period);
}
