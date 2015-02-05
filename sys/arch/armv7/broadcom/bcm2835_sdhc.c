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
#include <machine/bus.h>
#include <machine/fdt.h>
#include <arm/cpufunc.h>
#include <armv7/armv7/armv7var.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

struct bcm_sdhc_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	void			*sc_ih;
};

int	 bcm_sdhc_match(struct device *, void *, void *);
void	 bcm_sdhc_attach(struct device *, struct device *, void *);

struct cfattach	bcmsdhc_ca = {
	sizeof (struct bcm_sdhc_softc), bcm_sdhc_match, bcm_sdhc_attach
};

struct cfdriver bcmsdhc_cd = {
	NULL, "bcmsdhc", DV_DULL
};

int
bcm_sdhc_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("brcm,bcm2835-sdhci", aa->aa_node) ||
	    fdt_node_compatible("broadcom,bcm2835-sdhci", aa->aa_node))
		return (1);

	return 0;
}

void
bcm_sdhc_attach(struct device *parent, struct device *self, void *args)
{
	struct bcm_sdhc_softc *sc = (struct bcm_sdhc_softc *)self;
	struct armv7_attach_args *aa = args;
	struct sdhc_softc *ssc;
	struct fdt_memory mem;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	if ((ssc = (struct sdhc_softc *)config_found(self, NULL, NULL)) == NULL)
		goto mem;

	ssc->sc_flags = 0;
	ssc->sc_flags |= SDHC_F_32BIT_ACCESS;
	ssc->sc_flags |= SDHC_F_HOSTCAPS;
	ssc->sc_caps = SDHC_VOLTAGE_SUPP_3_3V | SDHC_HIGH_SPEED_SUPP |
	    (SDHC_MAX_BLK_LEN_1024 << SDHC_MAX_BLK_LEN_SHIFT);
	ssc->sc_clkbase = 50000; /* Default to 50MHz */

	/* TODO: Fetch frequency from FDT. */

	/* Allocate an array big enough to hold all the possible hosts */
	ssc->sc_host = mallocarray(1, sizeof(struct sdhc_host *),
	    M_DEVBUF, M_WAITOK);

	sc->sc_ih = arm_intr_establish_fdt(aa->aa_node, IPL_SDMMC, sdhc_intr,
	    ssc, ssc->sc_dev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    ssc->sc_dev.dv_xname);
		goto mem;
	}

	if (sdhc_host_found(ssc, sc->sc_iot, sc->sc_ioh, mem.size) != 0) {
		/* XXX: sc->sc_host leak */
		printf("%s: can't initialize host\n", ssc->sc_dev.dv_xname);
		goto intr;
	}

	return;

intr:
	arm_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
mem:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, mem.size);
}

int	sdhc_bcm_match(struct device *, void *, void *);
void	sdhc_bcm_attach(struct device *, struct device *, void *);

struct cfattach sdhc_bcm_ca = {
	sizeof (struct sdhc_softc), sdhc_bcm_match, sdhc_bcm_attach
};

int
sdhc_bcm_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
sdhc_bcm_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");
}
