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
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

#define SARL				0x00
#define 	SARL_PCLK_FREQ_OPT		21
#define 	SARL_PCLK_FREQ_OPT_MASK		0x7
#define 	SARL_FAB_FREQ_OPT		24
#define 	SARL_FAB_FREQ_OPT_MASK		0xf
#define SARH				0x04
#define 	SARH_PCLK_FREQ_OPT		52
#define 	SARH_PCLK_FREQ_OPT_MASK		0x1
#define 	SARH_PCLK_FREQ_OPT_SHIFT	3
#define 	SARH_FAB_FREQ_OPT		51
#define 	SARH_FAB_FREQ_OPT_MASK		0x1
#define 	SARH_FAB_FREQ_OPT_SHIFT		4

#define SAR_CPU_FREQ(sar)		((((sar >> 52) & 0x1) << 3) | \
					    ((sar >> 21) & 0x7))
#define SAR_FAB_FREQ(sar)		((((sar >> 51) & 0x1) << 4) | \
					    ((sar >> 24) & 0xf))

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

static const uint32_t mvaxpcc_cpu_freqs[] = {
	1000000, 1066000, 1200000, 1333000, 1500000,
	1666000, 1800000, 2000000,  600000,  667000,
	 800000, 1600000, 2133000, 2200000, 2400000
};

static const int mvaxpcc_nbclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 2}, {2, 2},
	{1, 2}, {1, 2}, {1, 1}, {2, 3},
	{0, 1}, {1, 2}, {2, 4}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {2, 2},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int mvaxpcc_hclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 6}, {2, 3},
	{1, 3}, {1, 4}, {1, 2}, {2, 6},
	{0, 1}, {1, 6}, {2, 10}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 2},
	{2, 6}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int mvaxpcc_dramclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 3}, {2, 3},
	{1, 3}, {1, 2}, {1, 2}, {2, 6},
	{0, 1}, {1, 3}, {2, 5}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

struct mvaxpcc_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	struct clk		*sc_clk[5];
};

int	 mvaxpcc_match(struct device *, void *, void *);
void	 mvaxpcc_attach(struct device *, struct device *, void *);
static struct clk *mvaxpcc_provider_get(void *, void *, int *, int);
int	 mvaxpcc_cpuspeed(int *);

struct cfattach	mvaxpcc_ca = {
	sizeof (struct mvaxpcc_softc), mvaxpcc_match, mvaxpcc_attach
};

struct cfdriver mvaxpcc_cd = {
	NULL, "mvaxpcc", DV_DULL
};

int
mvaxpcc_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-xp-core-clock", aa->aa_node))
		return (1);

	return 0;
}

void
mvaxpcc_attach(struct device *parent, struct device *self, void *args)
{
	struct mvaxpcc_softc *sc = (struct mvaxpcc_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	uint32_t cpu, fab;
	uint64_t sar;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	sar = (HREAD4(sc, SARH) << 31) | HREAD4(sc, SARL);
	cpu = SAR_CPU_FREQ(sar);
	fab = SAR_FAB_FREQ(sar);

	if (cpu > nitems(mvaxpcc_cpu_freqs))
		panic("%s: invalid cpu frequency", __func__);

	sc->sc_clk[0] = clk_fixed_rate("tclk", 250000);
	sc->sc_clk[1] = clk_fixed_rate("cpuclk", mvaxpcc_cpu_freqs[cpu]);
	sc->sc_clk[2] = clk_fixed_factor("nbclk", sc->sc_clk[1],
	    mvaxpcc_nbclk_ratios[fab][0], mvaxpcc_nbclk_ratios[fab][1]);
	sc->sc_clk[3] = clk_fixed_factor("hclk", sc->sc_clk[1],
	    mvaxpcc_hclk_ratios[fab][0], mvaxpcc_hclk_ratios[fab][1]);
	sc->sc_clk[4] = clk_fixed_factor("dramclk", sc->sc_clk[1],
	    mvaxpcc_dramclk_ratios[fab][0], mvaxpcc_dramclk_ratios[fab][1]);

	clk_fdt_register_provider(aa->aa_node, mvaxpcc_provider_get, self);

	cpu_cpuspeed = mvaxpcc_cpuspeed;
}

static struct clk *
mvaxpcc_provider_get(void *self, void *node, int *args, int narg)
{
	struct mvaxpcc_softc *sc = (struct mvaxpcc_softc *)self;

	if (narg != 1 || *args > nitems(sc->sc_clk))
		return NULL;

	return sc->sc_clk[*args];
}

int
mvaxpcc_cpuspeed(int *freq)
{
	*freq = clk_get_rate(clk_get("cpuclk")) / 1000;
	return (0);
}
