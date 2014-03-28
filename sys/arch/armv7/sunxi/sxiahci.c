/*	$OpenBSD	*/
/*
 * Copyright (c) 2013 Patrick Wildt <patrick@blueri.se>
 * Copyright (c) 2013,2014 Artturi Alm
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
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ic/ahcivar.h>
#include <dev/ic/ahcireg.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/sunxi/sunxireg.h>
#include <armv7/sunxi/sxiccmuvar.h>
#include <armv7/sunxi/sxipiovar.h>

#define	SXIAHCI_CAP		0x00
#define	SXIAHCI_GHC		0x04
#define	SXIAHCI_PI		0x0c
#define	SXIAHCI_PHYCS0		0xc0
#define	SXIAHCI_PHYCS1		0xc4
#define	SXIAHCI_PHYCS2		0xc8
#define	SXIAHCI_TIMER1MS	0xe0
#define	SXIAHCI_RWC		0xfc

#define	SXIAHCI_TIMEOUT		0x100000

void	sxiahci_attach(struct device *, struct device *, void *);

struct cfattach sxiahci_ca = {
	sizeof(struct ahci_softc), NULL, sxiahci_attach
};

struct cfdriver sxiahci_cd = {
	NULL, "ahci", DV_DULL
};

extern int ahci_intr(void *);

void
sxiahci_attach(struct device *parent, struct device *self, void *args)
{
	struct ahci_softc *sc = (struct ahci_softc *)self;
	struct armv7_attach_args *aa = args;
	uint32_t timo;

	sc->sc_iot = aa->aa_iot;
	sc->sc_ios = aa->aa_dev->mem[0].size;
	sc->sc_dmat = aa->aa_dmat;

	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    sc->sc_ios, 0, &sc->sc_ioh))
		panic("sxiahci_attach: bus_space_map failed!");

	/* enable clock */
	sxiccmu_enablemodule(CCMU_AHCI);

	/* power up phy */
	sxipio_setcfg(SXIPIO_SATA_PWR, SXIPIO_OUTPUT);
	sxipio_setpin(SXIPIO_SATA_PWR);
	delay(5000);

	/* setup magix */
	SXIWRITE4(sc, SXIAHCI_RWC, 0);
	delay(5000);

	SXISET4(sc, SXIAHCI_PHYCS1, 1 << 19);

	SXICMS4(sc, SXIAHCI_PHYCS0,
	    7 << 24,
	    5 << 24 | 1 << 23 | 1 << 18);

	SXICMS4(sc, SXIAHCI_PHYCS1,
	    3 << 16 | 0x1f << 8 | 3 << 6,
	    2 << 16 | 0x06 << 8 | 2 << 6);
	SXISET4(sc, SXIAHCI_PHYCS1, 1 << 28 | 1 << 15);
	SXICLR4(sc, SXIAHCI_PHYCS1, 1 << 19);

	SXICMS4(sc, SXIAHCI_PHYCS0,
	    7 << 20,
	    3 << 20);
	SXICMS4(sc, SXIAHCI_PHYCS2,
	    0x1f << 5,
	    0x19 << 5);
	delay(5000);

	SXISET4(sc, SXIAHCI_PHYCS0, 1 << 19);
	delay(20);

	timo = SXIAHCI_TIMEOUT;
	while ((SXIREAD4(sc, SXIAHCI_PHYCS0) >> 28 & 7) != 2 && --timo)
		delay(10);
	if (!timo) {
		printf(": AHCI phy power up failed.\n");
		goto dismod;
	}

	SXISET4(sc, SXIAHCI_PHYCS2, 1 << 24);

	timo = SXIAHCI_TIMEOUT;
	while ((SXIREAD4(sc, SXIAHCI_PHYCS2) & (1 << 24)) && --timo)
		delay(10);
	if (!timo) {
		printf(": AHCI phy calibration failed.\n");
		goto dismod;
	}

	delay(15000);
	SXIWRITE4(sc, SXIAHCI_RWC, 7);

	sc->sc_ih = arm_intr_establish(aa->aa_dev->irq[0], IPL_BIO,
	    ahci_intr, sc, sc->sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		goto clrpwr;
	}

	SXIWRITE4(sc, SXIAHCI_PI, 1);
	SXICLR4(sc, SXIAHCI_CAP, AHCI_REG_CAP_SPM);
	sc->sc_flags |= AHCI_F_NO_PMP | AHCI_F_SUNXI_QUIRK;
	if (ahci_attach(sc) != 0) {
		/* error printed by ahci_attach */
		goto irq;
	}

	return;
irq:
	arm_intr_disestablish(sc->sc_ih);
clrpwr:
	sxipio_clrpin(SXIPIO_SATA_PWR);
dismod:
	sxiccmu_disablemodule(CCMU_AHCI);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
}
