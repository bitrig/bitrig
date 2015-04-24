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

struct clk_mem mvaxpgc_clk_mem;

struct mvaxpgc_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	struct clk		*sc_clk[29];
};

int	 mvaxpgc_match(struct device *, void *, void *);
void	 mvaxpgc_attach(struct device *, struct device *, void *);
static struct clk *mvaxpgc_provider_get(void *, void *, int *, int);

struct cfattach	mvaxpgc_ca = {
	sizeof (struct mvaxpgc_softc), mvaxpgc_match, mvaxpgc_attach
};

struct cfdriver mvaxpgc_cd = {
	NULL, "mvaxpgc", DV_DULL
};

int
mvaxpgc_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-xp-gating-clock", aa->aa_node))
		return (1);

	return 0;
}

void
mvaxpgc_attach(struct device *parent, struct device *self, void *args)
{
	struct mvaxpgc_softc *sc = (struct mvaxpgc_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	struct clk *pclk;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	mvaxpgc_clk_mem.iot = sc->sc_iot;
	mvaxpgc_clk_mem.ioh = sc->sc_ioh;

	printf("\n");

	pclk = clk_fdt_get(aa->aa_node, 0);
	if (pclk == NULL)
		panic("%s: cant retrieve parent clock", __func__);

	sc->sc_clk[0] = clk_gate("audio", pclk, 0x0, 0, &mvaxpgc_clk_mem);
	sc->sc_clk[1] = clk_gate("ge3", pclk, 0x0, 1, &mvaxpgc_clk_mem);
	sc->sc_clk[2] = clk_gate("ge2", pclk, 0x0, 2, &mvaxpgc_clk_mem);
	sc->sc_clk[3] = clk_gate("ge1", pclk, 0x0, 3, &mvaxpgc_clk_mem);
	sc->sc_clk[4] = clk_gate("ge0", pclk, 0x0, 4, &mvaxpgc_clk_mem);
	sc->sc_clk[5] = clk_gate("pex00", pclk, 0x0, 5, &mvaxpgc_clk_mem);
	sc->sc_clk[6] = clk_gate("pex01", pclk, 0x0, 6, &mvaxpgc_clk_mem);
	sc->sc_clk[7] = clk_gate("pex02", pclk, 0x0, 7, &mvaxpgc_clk_mem);
	sc->sc_clk[8] = clk_gate("pex03", pclk, 0x0, 8, &mvaxpgc_clk_mem);
	sc->sc_clk[9] = clk_gate("pex10", pclk, 0x0, 9, &mvaxpgc_clk_mem);
	sc->sc_clk[10] = clk_gate("pex11", pclk, 0x0, 10, &mvaxpgc_clk_mem);
	sc->sc_clk[11] = clk_gate("pex12", pclk, 0x0, 12, &mvaxpgc_clk_mem);
	sc->sc_clk[12] = clk_gate("pex13", pclk, 0x0, 12, &mvaxpgc_clk_mem);
	sc->sc_clk[13] = clk_gate("bp", pclk, 0x0, 13, &mvaxpgc_clk_mem);
	sc->sc_clk[14] = clk_gate("sata0lnk", pclk, 0x0, 14, &mvaxpgc_clk_mem);
	sc->sc_clk[15] = clk_gate("sata0", clk_get("sata0lnk"), 0x0, 15, &mvaxpgc_clk_mem);
	sc->sc_clk[16] = clk_gate("lcd", pclk, 0x0, 16, &mvaxpgc_clk_mem);
	sc->sc_clk[17] = clk_gate("sdio", pclk, 0x0, 17, &mvaxpgc_clk_mem);
	sc->sc_clk[18] = clk_gate("usb0", pclk, 0x0, 18, &mvaxpgc_clk_mem);
	sc->sc_clk[19] = clk_gate("usb1", pclk, 0x0, 19, &mvaxpgc_clk_mem);
	sc->sc_clk[20] = clk_gate("usb2", pclk, 0x0, 20, &mvaxpgc_clk_mem);
	sc->sc_clk[21] = clk_gate("xor0", pclk, 0x0, 22, &mvaxpgc_clk_mem);
	sc->sc_clk[22] = clk_gate("crypto", pclk, 0x0, 23, &mvaxpgc_clk_mem);
	sc->sc_clk[23] = clk_gate("tdm", pclk, 0x0, 25, &mvaxpgc_clk_mem);
	sc->sc_clk[24] = clk_gate("pex20", pclk, 0x0, 26, &mvaxpgc_clk_mem);
	sc->sc_clk[25] = clk_gate("pex30", pclk, 0x0, 27, &mvaxpgc_clk_mem);
	sc->sc_clk[26] = clk_gate("xor1", pclk, 0x0, 28, &mvaxpgc_clk_mem);
	sc->sc_clk[27] = clk_gate("sata1lnk", pclk, 0x0, 29, &mvaxpgc_clk_mem);
	sc->sc_clk[28] = clk_gate("sata1", clk_get("sata1lnk"), 0x0, 30, &mvaxpgc_clk_mem);

	clk_fdt_register_provider(aa->aa_node, mvaxpgc_provider_get, self);
}

static struct clk *
mvaxpgc_provider_get(void *self, void *node, int *args, int narg)
{
	struct mvaxpgc_softc *sc = (struct mvaxpgc_softc *)self;

	if (narg != 1 || *args > nitems(sc->sc_clk))
		return NULL;

	return sc->sc_clk[*args];
}
