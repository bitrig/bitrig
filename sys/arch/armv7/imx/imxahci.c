/*
 * Copyright (c) 2013 Patrick Wildt <patrick@blueri.se>
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
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/fdt.h>
#include <machine/clock.h>

#include <dev/ic/ahcireg.h>
#include <dev/ic/ahcivar.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxccmvar.h>
#include <armv7/imx/imxiomuxcvar.h>

/* registers */
#define SATA_CAP		0x000
#define SATA_GHC		0x004
#define SATA_IS			0x008
#define SATA_PI			0x00C
#define SATA_VS			0x010
#define SATA_CCC_CTL		0x014
#define SATA_CCC_PORTS		0x018
#define SATA_CAP2		0x024
#define SATA_BISTAFR		0x0A0
#define SATA_BISTCR		0x0A4
#define SATA_BISTFCTR		0x0A8
#define SATA_BSTSR		0x0AC
#define SATA_OOBR		0x0BC
#define SATA_GPCR		0x0D0
#define SATA_GPSR		0x0D4
#define SATA_TIMER1MS		0x0E0
#define SATA_TESTR		0x0F4
#define SATA_VERSIONR		0x0F8
#define SATA_P0CLB		0x100
#define SATA_P0FB		0x108
#define SATA_P0IS		0x110
#define SATA_P0IE		0x114
#define SATA_P0CMD		0x118
#define SATA_P0TFD		0x120
#define SATA_P0SIG		0x124
#define SATA_P0SSTS		0x128
#define SATA_P0SCTL		0x12C
#define SATA_P0SERR		0x130
#define SATA_P0SACT		0x134
#define SATA_P0CI		0x138
#define SATA_P0SNTF		0x13C
#define SATA_P0DMACR		0x170
#define SATA_P0PHYCR		0x178
#define SATA_P0PHYSR		0x17C

#define SATA_CAP_SSS		(1 << 27)
#define SATA_GHC_HR		(1 << 0)
#define SATA_P0PHYCR_TEST_PDDQ	(1 << 20)

int	imxahci_match(struct device *, void *, void *);
int	imxahci_fdt_match(struct device *, void *, void *);
void	imxahci_attach(struct device *, struct device *, void *);
int	imxahci_detach(struct device *, int);
int	imxahci_activate(struct device *, int);

extern int ahci_intr(void *);

struct imxahci_softc {
	struct device		 sc_dev;
	struct ahci_softc 	*sc_ahci;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	bus_size_t		 sc_ios;
	bus_dma_tag_t		 sc_dmat;
	void			*sc_ih;
	struct clk		*sc_clk;
};

struct cfattach imxahci_ca = {
	sizeof(struct imxahci_softc),
	imxahci_match,
	imxahci_attach,
	imxahci_detach,
};

struct cfattach imxahci_fdt_ca = {
	sizeof(struct imxahci_softc),
	imxahci_fdt_match,
	imxahci_attach,
	imxahci_detach,
};

struct cfdriver imxahci_cd = {
	NULL, "imxahci", DV_DULL
};

int
imxahci_match(struct device *parent, void *v, void *aux)
{
	/* AHCI is only available on Quad and Dual. */
	if (imx_is_mx6q())
		return 1;

	return 0;
}

int
imxahci_fdt_match(struct device *parent, void *v, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fsl,imx6q-ahci", aa->aa_node))
		return 1;

	return 0;
}

void
imxahci_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct imxahci_softc *sc = (struct imxahci_softc *) self;
	struct ahci_softc *asc;
	uint32_t timeout = 0x100000;
	struct fdt_memory mem;
	int irq;

	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	if (aa->aa_node) {
		uint32_t ints[3];

		if (fdt_get_memory_address(aa->aa_node, 0, &mem))
			panic("%s: could not extract memory data from FDT",
			    __func__);

		/* TODO: Add interrupt FDT API. */
		if (fdt_node_property_ints(aa->aa_node, "interrupts",
		    ints, 3) != 3)
			panic("%s: could not extract interrupt data from FDT",
			    __func__);

		irq = ints[1];
	} else {
		mem.addr = aa->aa_dev->mem[0].addr;
		mem.size = aa->aa_dev->mem[0].size;
		irq = aa->aa_dev->irq[0];
	}

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);
	sc->sc_ios = mem.size;

	sc->sc_clk = clk_get("ahb");
	if (sc->sc_clk == NULL) {
		printf(": can't get clock\n");
		goto unmap;
	}
	clk_enable(sc->sc_clk);

	/* power it up */
	clk_enable(clk_get("sata_ref_100m"));
	clk_enable(clk_get("sata"));
	delay(100);

	/* power phy up */
	imxiomuxc_enable_sata();

	/* setup */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR) & ~SATA_P0PHYCR_TEST_PDDQ);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_GHC, SATA_GHC_HR);

	while (!bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_VERSIONR));

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_CAP,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_CAP) | SATA_CAP_SSS);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_PI, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_TIMER1MS, clk_get_rate(sc->sc_clk));

	while (!(bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0SSTS) & 0xF) && timeout--);

	printf("\n");

	if ((asc = (struct ahci_softc *)config_found(self, NULL, NULL)) == NULL)
		goto unmap;

	sc->sc_ih = arm_intr_establish(irq, IPL_BIO,
	    ahci_intr, asc, asc->sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		goto unmap;
	}

	sc->sc_ahci = asc;
	asc->sc_iot = sc->sc_iot;
	asc->sc_ioh = sc->sc_ioh;
	asc->sc_ios = sc->sc_ios;
	asc->sc_ih = sc->sc_ih;
	asc->sc_dmat = aa->aa_dmat;
	if (ahci_attach(asc) != 0) {
		/* error printed by ahci_attach */
		goto irq;
	}

	return;
irq:
	arm_intr_disestablish(sc->sc_ih);
unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
}

int
imxahci_detach(struct device *self, int flags)
{
	struct imxahci_softc *sc = (struct imxahci_softc *) self;
	struct ahci_softc *asc = sc->sc_ahci;

	ahci_detach(asc, flags);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return 0;
}

int
imxahci_activate(struct device *self, int act)
{
	struct imxahci_softc *sc = (struct imxahci_softc *) self;
	struct ahci_softc *asc = sc->sc_ahci;

	return ahci_activate((struct device *)asc, act);
}

int	ahci_imx_match(struct device *, void *, void *);
void	ahci_imx_attach(struct device *, struct device *, void *);

struct cfattach ahci_imx_ca = {
	sizeof(struct ahci_softc),
	ahci_imx_match,
	ahci_imx_attach,
};

int
ahci_imx_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
ahci_imx_attach(struct device *parent, struct device *self, void *args)
{
}
