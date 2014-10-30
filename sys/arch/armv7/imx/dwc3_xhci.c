/*-
 * Copyright (c) 2014 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#include <armv7/armv7/armv7var.h>

#define	GSNPSID				0xc120
#define	 GSNPSID_MASK			0xffff0000
#define	 REVISION_MASK			0xffff
#define	GCTL				0xc110
#define	 GCTL_PWRDNSCALE(n)		((n) << 19)
#define	 GCTL_U2RSTECN			(1 << 16)
#define	 GCTL_CLK_BUS			(0)
#define	 GCTL_CLK_PIPE			(1)
#define	 GCTL_CLK_PIPEHALF		(2)
#define	 GCTL_CLK_M			(3)
#define	 GCTL_CLK_S			(6)
#define	 GCTL_PRTCAP(n)			(((n) & (3 << 12)) >> 12)
#define	 GCTL_PRTCAPDIR(n)		((n) << 12)
#define	 GCTL_PRTCAP_HOST		1
#define	 GCTL_PRTCAP_DEVICE		2
#define	 GCTL_PRTCAP_OTG		3
#define	 GCTL_CORESOFTRESET		(1 << 11)
#define	 GCTL_SCALEDOWN_MASK		3
#define	 GCTL_SCALEDOWN_SHIFT		4
#define	 GCTL_DISSCRAMBLE		(1 << 3)
#define	 GCTL_DSBLCLKGTNG		(1 << 0)
#define	GHWPARAMS1			0xc13c
#define	 GHWPARAMS1_EN_PWROPT(n)	(((n) & (3 << 24)) >> 24)
#define	 GHWPARAMS1_EN_PWROPT_NO	0
#define	 GHWPARAMS1_EN_PWROPT_CLK	1
#define	GFLADJ				0xc630
#define	 GFLADJ_30MHZ_MASK		0xff
#define	 GFLADJ_30MHZ_SHIFT		0
#define	 GFLADJ_30MHZ_REG_SEL		(1 << 7)

#define DEVNAME(sc)	(sc)->sc_bus.bdev.dv_xname

/* Forward declarations */
static int	dwc3_xhci_match(struct device *, void *, void *);
static void	dwc3_xhci_attach(struct device *, struct device *, void *);
static int	dwc3_xhci_detach(struct device *, int);

struct dwc3_xhci_softc {
	struct device		sc_dev;
	struct xhci_softc	*sc;
	void			*sc_node;
	void			*sc_ih;
	bus_space_handle_t	uh_ioh;
	bus_space_handle_t	ph_ioh;
	bus_space_handle_t	nc_ioh;
};

struct cfdriver dwcthree_cd = {
	NULL, "dwcthree", DV_DULL
};

struct cfattach dwcthree_ca = {
	sizeof (struct dwc3_xhci_softc),
	dwc3_xhci_match,
	dwc3_xhci_attach,
	dwc3_xhci_detach,
	NULL
};

/*
 * Public methods
 */
static int
dwc3_xhci_match(struct device *parent, void *self, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("snps,dwc3", aa->aa_node))
		return 1;

	return 0;
}

static int
dwc3_init(struct xhci_softc *sc)
{
	int hwparams1;
	int rev;
	int reg;

	rev = XREAD4(sc, GSNPSID);
	if ((rev & GSNPSID_MASK) != 0x55330000) {
		printf("%s: It is not DWC3 controller\n", DEVNAME(sc));
		return (-1);
	}

	/* Reset controller */
	XWRITE4(sc, GCTL, GCTL_CORESOFTRESET);
	reg = XREAD4(sc, GCTL);
	reg &= ~GCTL_CORESOFTRESET;
	XWRITE4(sc, GCTL, reg);

	hwparams1 = XREAD4(sc, GHWPARAMS1);

	reg = XREAD4(sc, GCTL);
	reg &= ~(GCTL_SCALEDOWN_MASK << GCTL_SCALEDOWN_SHIFT);
	reg &= ~(GCTL_DISSCRAMBLE);

	if (GHWPARAMS1_EN_PWROPT(hwparams1) == \
	    GHWPARAMS1_EN_PWROPT_CLK)
		reg &= ~(GCTL_DSBLCLKGTNG);

	if ((rev & REVISION_MASK) < 0x190a)
		reg |= (GCTL_U2RSTECN);
	XWRITE4(sc, GCTL, reg);

	/* Set host mode */
	reg = XREAD4(sc, GCTL);
	reg &= ~(GCTL_PRTCAPDIR(GCTL_PRTCAP_OTG));
	reg |= GCTL_PRTCAPDIR(GCTL_PRTCAP_HOST);
	XWRITE4(sc, GCTL, reg);

	/* LS1021A: Errata A-009116 */
	reg = XREAD4(sc, GFLADJ);
	reg &= ~(GFLADJ_30MHZ_MASK);
	reg |= (GFLADJ_30MHZ_REG_SEL | (0x20 << GFLADJ_30MHZ_SHIFT));
	XWRITE4(sc, GFLADJ, reg);

	return (0);
}

static void
dwc3_xhci_attach(struct device *parent, struct device *self, void *aux)
{
	struct dwc3_xhci_softc	*sc = (struct dwc3_xhci_softc *)self;
	struct armv7_attach_args *aa = aux;
	struct fdt_memory	mem;
	int			error;

	sc->sc_node = aa->aa_node;
	if (sc->sc_node == NULL)
		panic("%s: no fdt node", __func__);

	if (fdt_get_memory_address(sc->sc_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	printf("\n");

	if ((sc->sc = (struct xhci_softc *)config_found(self, NULL, NULL)) == NULL)
		goto out;

	sc->sc->iot = aa->aa_iot;
	sc->sc->sc_bus.dmatag = aa->aa_dmat;
	sc->sc->sc_size = mem.size;

	if (bus_space_map(sc->sc->iot, mem.addr, mem.size, 0, &sc->sc->ioh)) {
		printf("%s: cannot map mem space\n", __func__);
		goto out;
	}

	sc->sc_ih = arm_intr_establish_fdt(sc->sc_node, IPL_USB, xhci_intr,
	    sc->sc, DEVNAME(sc->sc));
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		goto mem;
	}

	strlcpy(sc->sc->sc_vendor, "Synopsys", sizeof(sc->sc->sc_vendor));

	dwc3_init(sc->sc);

	if ((error = xhci_init(sc->sc)) != 0) {
		printf("%s: init failed, error=%d\n",
		    DEVNAME(sc->sc), error);
		goto irq;
	}

	printf("\n");

	/* Attach usb device. */
	config_found((struct device *)sc->sc, &sc->sc->sc_bus, usbctlprint);

	/* Now that the stack is ready, config' the HC and enable interrupts. */
	xhci_config(sc->sc);

	return;

irq:
	arm_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
mem:
	bus_space_unmap(sc->sc->iot, sc->sc->ioh, sc->sc->sc_size);
	sc->sc->sc_size = 0;
out:
	return;
}

static int
dwc3_xhci_detach(struct device *self, int flags)
{
	struct dwc3_xhci_softc		*sc = (struct dwc3_xhci_softc *)self;
	int				rv;

	if (sc->sc == NULL)
		return (0);

	rv = xhci_detach((struct device *)sc->sc, flags);
	if (rv)
		return (rv);

	if (sc->sc_ih != NULL) {
		arm_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc->sc_size) {
		bus_space_unmap(sc->sc->iot, sc->sc->ioh, sc->sc->sc_size);
		sc->sc->sc_size = 0;
	}

	return (0);
}

int	xhci_dwcthree_match(struct device *, void *, void *);
void	xhci_dwcthree_attach(struct device *, struct device *, void *);

struct cfattach xhci_dwcthree_ca = {
	sizeof (struct xhci_softc), xhci_dwcthree_match, xhci_dwcthree_attach
};

int
xhci_dwcthree_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
xhci_dwcthree_attach(struct device *parent, struct device *self, void *aux)
{
}
