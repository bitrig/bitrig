/*	$OpenBSD: msmehci.c,v 1.10 2016/11/22 11:03:08 kettenis Exp $ */

/*
 * Copyright (c) 2005 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2016 Mark Kettenis <kettenis@openbsd.org>
 * Copyright (c) 2016 Patrick Wildt <patrick@blueri.se>
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
#include <sys/kernel.h>
#include <sys/rwlock.h>
#include <sys/timeout.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/fdt.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_clock.h>
#include <dev/ofw/ofw_pinctrl.h>
#include <dev/ofw/ofw_regulator.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#include <arm64/arm64/arm64var.h>

#define MSMEHCI_AHBBURST				0x090
#define MSMEHCI_AHBMODE					0x098
#define MSMEHCI_GENCONFIG_2				0x0a0
#define  MSMEHCI_GENCONFIG_2_ULPI_TX_PKT_EN_CLR_FIX	(1 << 19)
#define MSMEHCI_PORTSC					0x184
#define  MSMEHCI_PORTSC_PTS_ULPI			(2 << 30)
#define MSMEHCI_USBMODE					0x1a8


struct msmehci_softc {
	struct ehci_softc	sc;
	int			sc_node;
	void			*sc_ih;
	bus_space_handle_t	sc_ioh;
};

int	msmehci_match(struct device *, void *, void *);
void	msmehci_attach(struct device *, struct device *, void *);
int	msmehci_detach(struct device *, int);

struct cfattach msmehci_ca = {
	sizeof(struct msmehci_softc), msmehci_match, msmehci_attach,
	msmehci_detach
};

struct cfdriver msmehci_cd = {
	NULL, "msmehci", DV_DULL
};

int
msmehci_match(struct device *parent, void *match, void *aux)
{
	struct arm64_attach_args *aa = aux;

	if (fdt_node_compatible("qcom,ehci-host", aa->aa_node))
		return 1;

	return 0;
}

int
msmehci_init_after_reset(struct ehci_softc *sc)
{
	EWRITE4(sc, MSMEHCI_PORTSC, MSMEHCI_PORTSC_PTS_ULPI);
	EWRITE4(sc, MSMEHCI_AHBBURST, 0);
	EWRITE4(sc, MSMEHCI_AHBMODE, 0x8);
	EWRITE4(sc, MSMEHCI_USBMODE, 0x3);
	EWRITE4(sc, MSMEHCI_GENCONFIG_2, 1 << 7);

	return 0;
}

void
msmehci_attach(struct device *parent, struct device *self, void *aux)
{
	struct msmehci_softc	*sc = (struct msmehci_softc *)self;
	struct arm64_attach_args	*aa = aux;
	usbd_status		 r;
	char			*devname = sc->sc.sc_bus.bdev.dv_xname;
	struct fdt_memory	mem;

	sc->sc_node = (int)aa->aa_node;
	sc->sc.iot = aa->aa_iot;
	sc->sc.sc_bus.dmatag = aa->aa_dmat;


	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);
	sc->sc.sc_size = mem.size;

	if (bus_space_map(sc->sc.iot, mem.addr,
	    mem.size, 0, &sc->sc_ioh)) {
		printf(": cannot map mem space\n");
		goto out;
	}
	if (bus_space_subregion(sc->sc.iot, sc->sc_ioh, 0x100, 0, &sc->sc.ioh)){
		printf(": cannot submap mem space\n");
		goto out;
	}
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);
	sc->sc.sc_init_after_reset = msmehci_init_after_reset;

	printf("\n");

	clock_enable_all(sc->sc_node);
	reset_deassert_all(sc->sc_node);

	/* Disable interrupts, so we don't get any spurious ones. */
	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);

	sc->sc_ih = arm_intr_establish_fdt(aa->aa_node, IPL_USB,
	    ehci_intr, &sc->sc, devname);
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		printf("XXX - disable ehci");
		goto mem0;
	}

	strlcpy(sc->sc.sc_vendor, "Qualcomm", sizeof(sc->sc.sc_vendor));
	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		printf("XXX - disable ehci");
		goto intr;
	}

	printf("\n");

	config_found(self, &sc->sc.sc_bus, usbctlprint);

	goto out;

intr:
	arm_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
mem0:
	bus_space_unmap(sc->sc.iot, sc->sc_ioh, sc->sc.sc_size);
	sc->sc.sc_size = 0;
out:
	return;
}

int
msmehci_detach(struct device *self, int flags)
{
	struct msmehci_softc		*sc = (struct msmehci_softc *)self;
	int				rv;

	rv = ehci_detach(self, flags);
	if (rv)
		return (rv);

	if (sc->sc_ih != NULL) {
		arm_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}

	return (0);
}

#if 0
int     ehci_msmehci_match(struct device *, void *, void *);
void    ehci_msmehci_attach(struct device *, struct device *, void *);

struct cfattach ehci_msmehci_ca = {
        sizeof (struct ehci_softc), ehci_msmehci_match, ehci_msmehci_attach
};

int
ehci_msmehci_match(struct device *parent, void *v, void *aux)
{
        return 1;
}

void
ehci_msmehci_attach(struct device *parent, struct device *self, void *aux)
{
}

#endif
