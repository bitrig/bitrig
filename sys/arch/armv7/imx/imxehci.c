/*	$OpenBSD: imxehci.c,v 1.4 2014/05/19 13:11:31 mpi Exp $ */
/*
 * Copyright (c) 2012-2013 Patrick Wildt <patrick@blueri.se>
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
#include <machine/clock.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxccmvar.h>
#include <armv7/imx/imxgpiovar.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

/* usb phy */
#define USBPHY_PWD			0x00
#define USBPHY_CTRL			0x30
#define USBPHY_CTRL_SET			0x34
#define USBPHY_CTRL_CLR			0x38
#define USBPHY_CTRL_TOG			0x3c

#define USBPHY_CTRL_ENUTMILEVEL2	(1 << 14)
#define USBPHY_CTRL_ENUTMILEVEL3	(1 << 15)
#define USBPHY_CTRL_CLKGATE		(1 << 30)
#define USBPHY_CTRL_SFTRST		(1U << 31)

/* ehci */
#define EHCI_USBMODE			0xa8

#define EHCI_USBMODE_HOST		(3 << 0)
#define EHCI_PS_PTS_UTMI_MASK	((1 << 25) | (3 << 30))

/* usb non-core */
#define USBNC_USB_OTG_CTRL		0x00
#define USBNC_USB_UH1_CTRL		0x04

#define USBNC_USB_OTG_CTRL_OVER_CUR_POL	(1 << 8)
#define USBNC_USB_OTG_CTRL_OVER_CUR_DIS	(1 << 7)
#define USBNC_USB_UH1_CTRL_OVER_CUR_POL	(1 << 8)
#define USBNC_USB_UH1_CTRL_OVER_CUR_DIS	(1 << 7)

/* port specific addresses */
#define USBOTG_ADDR	0x02184000
#define USBUH1_ADDR	0x02184200
#define USBUH2_ADDR	0x02184400
#define USBUH3_ADDR	0x02184600

/* board specific */
#define EHCI_HUMMINGBOARD_USB_H1_PWR		0
#define EHCI_HUMMINGBOARD_USB_OTG_PWR		(2*32+22)
#define EHCI_NITROGEN6X_USB_HUB_RST		(6*32+12)
#define EHCI_SABRESD_USB_PWR			(0*32+29)
#define EHCI_UTILITE_USB_HUB_RST		(6*32+8)

int	imxehci_match(struct device *, void *, void *);
void	imxehci_attach(struct device *, struct device *, void *);
int	imxehci_detach(struct device *, int);

struct imxehci_softc {
	struct device		sc_dev;
	void			*sc_ih;
	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_size_t		sc_size;
	u_int			sc_offs;
	bus_space_handle_t	uh_ioh;
	bus_space_handle_t	ph_ioh;
	bus_space_handle_t	nc_ioh;
};

struct imxehci_attach_args {
	bus_dma_tag_t		dmat;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	u_int			offs;
	int			irq;
};

struct cfdriver imxehci_cd = {
	NULL, "imxehci", DV_DULL
};

struct cfattach imxehci_ca = {
	sizeof (struct imxehci_softc), NULL, imxehci_attach, imxehci_detach
};
struct cfattach imxehci_fdt_ca = {
	sizeof (struct imxehci_softc), imxehci_match, imxehci_attach, imxehci_detach
};

int
imxehci_match(struct device *parent, void *v, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fsl,imx6q-usb", aa->aa_node))
		return 1;

	return 0;
}

void
imxehci_attach(struct device *parent, struct device *self, void *aux)
{
	struct imxehci_softc		*sc = (struct imxehci_softc *)self;
	struct ehci_softc		*esc;
	struct armv7_attach_args	*aa = aux;
	struct fdt_memory		 hmem, pmem, mmem;
	int				 irq, r;

	sc->iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	if (aa->aa_node) {
		uint32_t ints[3];
		void *node;

		if (fdt_get_memory_address(aa->aa_node, 0, &hmem))
			panic("%s: could not extract memory data from FDT",
			    __func__);

		node = fdt_find_node_by_phandle_prop(aa->aa_node, "fsl,usbphy");
		if (node == NULL || fdt_get_memory_address(node, 0, &pmem))
			panic("%s: could not extract phy data from FDT",
			    __func__);

		node = fdt_find_node_by_phandle_prop(aa->aa_node, "fsl,usbmisc");
		if (node == NULL || fdt_get_memory_address(node, 0, &mmem))
			panic("%s: could not extract phy data from FDT",
			    __func__);

		/* TODO: Add interrupt FDT API. */
		if (fdt_node_property_ints(aa->aa_node, "interrupts",
		    ints, 3) != 3)
			panic("%s: could not extract interrupt data from FDT",
			    __func__);

		irq = ints[1];
	} else {
		hmem.addr = aa->aa_dev->mem[0].addr;
		hmem.size = aa->aa_dev->mem[0].size;
		pmem.addr = aa->aa_dev->mem[1].addr;
		pmem.size = aa->aa_dev->mem[1].size;
		mmem.addr = aa->aa_dev->mem[2].addr;
		mmem.size = aa->aa_dev->mem[2].size;
		irq = aa->aa_dev->irq[0];
	}

	/* Map I/O space */
	if (bus_space_map(sc->iot, hmem.addr, hmem.size, 0, &sc->uh_ioh)) {
		printf(": cannot map mem space\n");
		goto hmem;
	}
	sc->ioh = sc->uh_ioh + 0x100;
	sc->sc_size = hmem.size;

	if (bus_space_map(sc->iot, pmem.addr, pmem.size, 0, &sc->ph_ioh)) {
		printf(": cannot map mem space\n");
		goto pmem;
	}

	if (bus_space_map(sc->iot, mmem.addr, mmem.size, 0, &sc->nc_ioh)) {
		printf(": cannot map mem space\n");
		goto mmem;
	}

	clk_enable(clk_get("usboh3"));
	delay(1000);

	if (hmem.addr == USBUH1_ADDR) {
		/* enable usb port power */
		switch (board_id)
		{
		case BOARD_ID_IMX6_CUBOXI:
		case BOARD_ID_IMX6_HUMMINGBOARD:
			imxgpio_set_bit(EHCI_HUMMINGBOARD_USB_H1_PWR);
			imxgpio_set_dir(EHCI_HUMMINGBOARD_USB_H1_PWR, IMXGPIO_DIR_OUT);
			delay(10);
			break;
		case BOARD_ID_IMX6_SABRELITE:
			imxgpio_clear_bit(EHCI_NITROGEN6X_USB_HUB_RST);
			imxgpio_set_dir(EHCI_NITROGEN6X_USB_HUB_RST, IMXGPIO_DIR_OUT);
			delay(1000 * 2);
			imxgpio_set_bit(EHCI_NITROGEN6X_USB_HUB_RST);
			delay(10);
			break;
		case BOARD_ID_IMX6_SABRESD:
			imxgpio_set_bit(EHCI_SABRESD_USB_PWR);
			imxgpio_set_dir(EHCI_SABRESD_USB_PWR, IMXGPIO_DIR_OUT);
			delay(10);
			break;
		case BOARD_ID_IMX6_UTILITE:
			imxgpio_clear_bit(EHCI_UTILITE_USB_HUB_RST);
			imxgpio_set_dir(EHCI_UTILITE_USB_HUB_RST, IMXGPIO_DIR_OUT);
			delay(10);
			imxgpio_set_bit(EHCI_UTILITE_USB_HUB_RST);
			delay(1000);
			break;
		}

		/* disable the carger detection, else signal on DP will be poor */
		imxccm_disable_usb2_chrg_detect();
		/* power host 1 */
		clk_enable(clk_get("pll7_usb_host"));
		clk_enable(clk_get("usbphy2_gate"));

		/* over current and polarity setting */
		bus_space_write_4(sc->iot, sc->nc_ioh, USBNC_USB_UH1_CTRL,
		    bus_space_read_4(sc->iot, sc->nc_ioh, USBNC_USB_UH1_CTRL) |
		    (USBNC_USB_UH1_CTRL_OVER_CUR_POL | USBNC_USB_UH1_CTRL_OVER_CUR_DIS));
	} else if (hmem.addr == USBOTG_ADDR) {
		/* enable usb port power */
		switch (board_id)
		{
		case BOARD_ID_IMX6_CUBOXI:
		case BOARD_ID_IMX6_HUMMINGBOARD:
			imxgpio_set_bit(EHCI_HUMMINGBOARD_USB_OTG_PWR);
			imxgpio_set_dir(EHCI_HUMMINGBOARD_USB_OTG_PWR, IMXGPIO_DIR_OUT);
			delay(10);
			break;
		}

		/* disable the carger detection, else signal on DP will be poor */
		imxccm_disable_usb1_chrg_detect();
		/* power host 0 */
		clk_enable(clk_get("pll3_usb_otg"));
		clk_enable(clk_get("usbphy1_gate"));

		/* over current and polarity setting */
		bus_space_write_4(sc->iot, sc->nc_ioh, USBNC_USB_OTG_CTRL,
		    bus_space_read_4(sc->iot, sc->nc_ioh, USBNC_USB_OTG_CTRL) |
		    (USBNC_USB_OTG_CTRL_OVER_CUR_POL | USBNC_USB_OTG_CTRL_OVER_CUR_DIS));
	}

	bus_space_write_4(sc->iot, sc->ph_ioh, USBPHY_CTRL_CLR,
	    USBPHY_CTRL_CLKGATE);

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);
	EOWRITE2(sc, EHCI_USBINTR, 0);

	/* Stop then Reset */
	uint32_t val = EOREAD4(sc, EHCI_USBCMD);
	val &= ~EHCI_CMD_RS;
	EOWRITE4(sc, EHCI_USBCMD, val);

	while (EOREAD4(sc, EHCI_USBCMD) & EHCI_CMD_RS)
		;

	val = EOREAD4(sc, EHCI_USBCMD);
	val |= EHCI_CMD_HCRESET;
	EOWRITE4(sc, EHCI_USBCMD, val);

	while (EOREAD4(sc, EHCI_USBCMD) & EHCI_CMD_HCRESET)
		;

	/* Reset USBPHY module */
	bus_space_write_4(sc->iot, sc->ph_ioh, USBPHY_CTRL_SET, USBPHY_CTRL_SFTRST);

	delay(10);

	/* Remove CLKGATE and SFTRST */
	bus_space_write_4(sc->iot, sc->ph_ioh, USBPHY_CTRL_CLR,
	    USBPHY_CTRL_CLKGATE | USBPHY_CTRL_SFTRST);

	delay(10);

	/* Power up the PHY */
	bus_space_write_4(sc->iot, sc->ph_ioh, USBPHY_PWD, 0);

	/* enable FS/LS device */
	bus_space_write_4(sc->iot, sc->ph_ioh, USBPHY_CTRL_SET,
	    USBPHY_CTRL_ENUTMILEVEL2 | USBPHY_CTRL_ENUTMILEVEL3);

	/* set host mode */
	EWRITE4(sc, EHCI_USBMODE,
	    EREAD4(sc, EHCI_USBMODE) | EHCI_USBMODE_HOST);

	/* set to UTMI mode */
	EOWRITE4(sc, EHCI_PORTSC(1),
	    EOREAD4(sc, EHCI_PORTSC(1)) & ~EHCI_PS_PTS_UTMI_MASK);

	printf("\n");

	if ((esc = (struct ehci_softc *)config_found(self, NULL, NULL)) == NULL)
		goto mmem;

	esc->iot = sc->iot;
	esc->ioh = sc->ioh;
	esc->sc_bus.dmatag = sc->sc_dmat;
	esc->sc_offs = sc->sc_offs;
	sc->sc_ih = arm_intr_establish(irq, IPL_USB,
	    ehci_intr, esc, esc->sc_bus.bdev.dv_xname);
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		return;
	}

	strlcpy(esc->sc_vendor, "i.MX6", sizeof(esc->sc_vendor));
	r = ehci_init(esc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n",
		    esc->sc_bus.bdev.dv_xname, r);
		goto intr;
	}

	printf("\n");

	config_found((struct device *)esc, &esc->sc_bus, usbctlprint);

	goto out;

intr:
	arm_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
mmem:
	bus_space_unmap(sc->iot, sc->nc_ioh, mmem.size);
pmem:
	bus_space_unmap(sc->iot, sc->ph_ioh, pmem.size);
hmem:
	bus_space_unmap(sc->iot, sc->uh_ioh, sc->sc_size);
	sc->sc_size = 0;
out:
	return;
}

int
imxehci_detach(struct device *self, int flags)
{
	struct imxehci_softc		*sc = (struct imxehci_softc *)self;
	int				 rv = 0;

	rv = ehci_detach(self, flags);
	if (rv)
		return (rv);

	if (sc->sc_ih != NULL) {
		arm_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc_size) {
		bus_space_unmap(sc->iot, sc->uh_ioh, sc->sc_size);
		sc->sc_size = 0;
	}

	return (0);
}

int	ehci_imx_match(struct device *, void *, void *);
void	ehci_imx_attach(struct device *, struct device *, void *);

struct cfattach ehci_imx_ca = {
	sizeof (struct ehci_softc), ehci_imx_match, ehci_imx_attach
};

int
ehci_imx_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
ehci_imx_attach(struct device *parent, struct device *self, void *aux)
{
}
