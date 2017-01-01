/*
 * Copyright (c) 2017 Dale Rahn <drahn@dalerahn.com>
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
#include <arm64/arm64/arm64var.h>


struct qgcc_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	struct clk		*sc_clk[5];
};

int	 qgcc_match(struct device *, void *, void *);
void	 qgcc_attach(struct device *, struct device *, void *);
static struct clk *qgcc_provider_get(void *, void *, int *, int);
int	 qgcc_cpuspeed(int *);

struct cfattach	qgcc_ca = {
	sizeof (struct qgcc_softc), qgcc_match, qgcc_attach
};

struct cfdriver qgcc_cd = {
	NULL, "qgcc", DV_DULL
};

int
qgcc_match(struct device *parent, void *cfdata, void *aux)
{
	struct arm64_attach_args *aa = aux;

	if (fdt_node_compatible("qcom,gcc-msm8916", aa->aa_node))
		return (1);

	return 0;
}

void
qgcc_attach(struct device *parent, struct device *self, void *args)
{
	struct qgcc_softc *sc = (struct qgcc_softc *)self;
	struct arm64_attach_args *aa = args;
	struct fdt_memory mem;


	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	/* XXX - these clocks are bogus */
	sc->sc_clk[0] = clk_fixed_rate("tclk", 250000);
#if 0
	sc->sc_clk[1] = clk_fixed_rate("cpuclk", qgcc_cpu_freqs[cpu]);
	sc->sc_clk[2] = clk_fixed_factor("nbclk", sc->sc_clk[1],
	    qgcc_nbclk_ratios[fab][0], qgcc_nbclk_ratios[fab][1]);
	sc->sc_clk[3] = clk_fixed_factor("hclk", sc->sc_clk[1],
	    qgcc_hclk_ratios[fab][0], qgcc_hclk_ratios[fab][1]);
	sc->sc_clk[4] = clk_fixed_factor("dramclk", sc->sc_clk[1],
	    qgcc_dramclk_ratios[fab][0], qgcc_dramclk_ratios[fab][1]);
#endif

	clk_fdt_register_provider(aa->aa_node, qgcc_provider_get, self);

	// XXX 
	//cpu_cpuspeed = qgcc_cpuspeed;
}

static struct clk *
qgcc_provider_get(void *self, void *node, int *args, int narg)
{
	struct qgcc_softc *sc = (struct qgcc_softc *)self;

	if (narg != 1 || *args > nitems(sc->sc_clk))
		return NULL;

	return sc->sc_clk[*args];
}

int
qgcc_cpuspeed(int *freq)
{
	*freq = clk_get_rate(clk_get("cpuclk")) / 1000;
	return (0);
}
