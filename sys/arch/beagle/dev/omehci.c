/*	$OpenBSD: omehci.c,v 1.14 2013/06/12 11:42:01 mpi Exp $ */

/*
 * Copyright (c) 2005 David Gwynne <dlg@openbsd.org>
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

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <beagle/dev/omapvar.h>
#include <beagle/dev/prcmvar.h>
#include <beagle/dev/omgpiovar.h>
#include <beagle/dev/omehcivar.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

void	omehci_attach(struct device *, struct device *, void *);
int	omehci_detach(struct device *, int);
int	omehci_activate(struct device *, int);

struct omehci_softc {
	struct ehci_softc	sc;
	void			*sc_ih;
	bus_space_handle_t	hs_ioh;
};

int omehci_init(struct omehci_softc *);
void omehci_soft_phy_reset(struct omehci_softc *sc, unsigned int port);
void	omehci_enable(struct omehci_softc *);
void	omehci_disable(struct omehci_softc *);
void omehci_utmi_init(struct omehci_softc *sc, unsigned int en_mask);
void misc_setup(struct omehci_softc *sc);
void omehci_phy_reset(uint32_t on, uint32_t _delay);
void omehci_uhh_init(struct omehci_softc *sc);
void mux_setup(struct omehci_softc *sc);
void do_mux(uint32_t offset, uint32_t mux, uint32_t mode);

#define OMAP_HS_USB_PORTS 3
uint32_t port_mode[OMAP_HS_USB_PORTS] = { EHCI_HCD_OMAP_MODE_PHY, EHCI_HCD_OMAP_MODE_UNKNOWN, EHCI_HCD_OMAP_MODE_UNKNOWN };

struct cfattach omehci_ca = {
	sizeof (struct omehci_softc), NULL, omehci_attach,
	omehci_detach, omehci_activate
};

void
omehci_attach(struct device *parent, struct device *self, void *aux)
{
	struct omehci_softc	*sc = (struct omehci_softc *)self;
	struct omap_attach_args	*oa = aux;
	usbd_status		r;
	char *devname = sc->sc.sc_bus.bdev.dv_xname;

	sc->sc.iot = oa->oa_iot;
	sc->sc.sc_bus.dmatag = oa->oa_dmat;
	sc->sc.sc_size = oa->oa_dev->mem[0].size;

	/* Map I/O space */
	if (bus_space_map(sc->sc.iot, oa->oa_dev->mem[0].addr,
		oa->oa_dev->mem[0].size, 0, &sc->sc.ioh)) {
		printf(": cannot map mem space\n");
		goto out;
	}

	if (bus_space_map(sc->sc.iot, oa->oa_dev->mem[1].addr,
		oa->oa_dev->mem[1].size, 0, &sc->hs_ioh)) {
		printf(": cannot map mem space\n");
		goto mem0;
	}

	printf("\n");

	omgpio_set_dir(OMAP_EHCI_PHY1_RESET_GPIO, OMGPIO_DIR_OUT);
	omgpio_set_dir(OMAP_EHCI_PHY2_RESET_GPIO, OMGPIO_DIR_OUT);
	omgpio_clear_bit(OMAP_EHCI_PHY1_RESET_GPIO);
	omgpio_clear_bit(OMAP_EHCI_PHY2_RESET_GPIO);
	delay(1000);

	mux_setup(sc);
	misc_setup(sc);
	/* wait until stable */
	delay(1000);

	omgpio_set_bit(OMAP_EHCI_PHY2_RESET_GPIO);
	/* wait until powered up */
	delay(1000);

	if (omehci_init(sc))
		return;

	omgpio_set_bit(OMAP_EHCI_PHY1_RESET_GPIO);
	delay(1000);

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);
	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);

	sc->sc_ih = arm_intr_establish(oa->oa_dev->irq[0], IPL_USB,
	    ehci_intr, &sc->sc, devname);
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		printf("XXX - disable ehci and prcm");
		goto mem1;
	}

	strlcpy(sc->sc.sc_vendor, "OMAP44xx", sizeof(sc->sc.sc_vendor));
	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		printf("XXX - disable ehci and prcm");
		goto intr;
	}

	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
	    usbctlprint);

	goto out;

intr:
	arm_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
mem1:
	bus_space_unmap(sc->sc.iot, sc->hs_ioh, oa->oa_dev->mem[1].size);
mem0:
	bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
	sc->sc.sc_size = 0;
out:
	return;
}

#define ALTCLKSRC 0x110
#define AUXCLK1 0x314
#define AUXCLK3 0x31c

/* AUXCLKx reg fields */
#define AUXCLK_ENABLE_MASK      (1 << 8)
#define AUXCLK_SRCSELECT_SHIFT      1
#define AUXCLK_SRCSELECT_MASK       (3 << 1)
#define AUXCLK_CLKDIV_SHIFT     16
#define AUXCLK_CLKDIV_MASK      (0xF << 16)

#define AUXCLK_SRCSELECT_SYS_CLK    0
#define AUXCLK_SRCSELECT_CORE_DPLL  1
#define AUXCLK_SRCSELECT_PER_DPLL   2
#define AUXCLK_SRCSELECT_ALTERNATE  3

#define AUXCLK_CLKDIV_2         1
#define AUXCLK_CLKDIV_16        0xF

/* ALTCLKSRC */
#define ALTCLKSRC_MODE_ACTIVE       1
#define ALTCLKSRC_MODE_MASK         3
#define ALTCLKSRC_ENABLE_INT_MASK   (1 << 2)
#define ALTCLKSRC_ENABLE_EXT_MASK   (1 << 3)

#define USBB1_ULPITLL_CLK      0x00c2
#define USBB1_ULPITLL_STP      0x00c4
#define USBB1_ULPITLL_DIR      0x00c6
#define USBB1_ULPITLL_NXT      0x00c8
#define USBB1_ULPITLL_DAT0     0x00ca
#define USBB1_ULPITLL_DAT1     0x00cc
#define USBB1_ULPITLL_DAT2     0x00ce
#define USBB1_ULPITLL_DAT3     0x00d0
#define USBB1_ULPITLL_DAT4     0x00d2
#define USBB1_ULPITLL_DAT5     0x00d4
#define USBB1_ULPITLL_DAT6     0x00d6
#define USBB1_ULPITLL_DAT7     0x00d8

#define OMAP_USB_MUX_ULPIPHY (4)
#define OMAP_PIN_OUTPUT (0 |OMAP_USB_MUX_ULPIPHY)
#define OMAP_PIN_INPUT_PULLDOWN ((1<<3) | (1<<8) | OMAP_USB_MUX_ULPIPHY)

void
do_mux(uint32_t offset, uint32_t mux, uint32_t mode)
{
	prcm_pcnf1_set_value(offset, mux | mode);
}

void
mux_setup(struct omehci_softc *sc)
{
	do_mux(USBB1_ULPITLL_CLK, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_STP, 4, OMAP_PIN_OUTPUT);
	do_mux(USBB1_ULPITLL_DIR, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_NXT, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT0, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT1, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT2, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT3, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT4, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT5, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT6, 4, OMAP_PIN_INPUT_PULLDOWN);
	do_mux(USBB1_ULPITLL_DAT7, 4, OMAP_PIN_INPUT_PULLDOWN);
}

void
misc_setup(struct omehci_softc *sc)
{
	uint32_t auxclk, altclksrc;

	/* ULPI PHY supplied by auxclk3 derived from sys_clk */
	//printf("ULPI PHY supplied by auxclk3\n");

	auxclk = prcm_scrm_get_value(AUXCLK3);
	/* Select sys_clk */
	auxclk &= ~AUXCLK_SRCSELECT_MASK;
	auxclk |=  AUXCLK_SRCSELECT_SYS_CLK << AUXCLK_SRCSELECT_SHIFT;
	/* Set the divisor to 2 */
	auxclk &= ~AUXCLK_CLKDIV_MASK;
	auxclk |= AUXCLK_CLKDIV_2 << AUXCLK_CLKDIV_SHIFT;
	/* Request auxilary clock #3 */
	auxclk |= AUXCLK_ENABLE_MASK;

	prcm_scrm_set_value(AUXCLK3, auxclk);

	altclksrc = prcm_scrm_get_value(ALTCLKSRC);

	/* Activate alternate system clock supplier */
	altclksrc &= ~ALTCLKSRC_MODE_MASK;
	altclksrc |= ALTCLKSRC_MODE_ACTIVE;

	/* enable clocks */
	altclksrc |= ALTCLKSRC_ENABLE_INT_MASK | ALTCLKSRC_ENABLE_EXT_MASK;

	prcm_scrm_set_value(ALTCLKSRC, altclksrc);
	prcm_pcnf2_set_value(0x58, prcm_pcnf2_get_value(0x58) & ~0xffff);

	return;
}

void omehci_phy_reset(uint32_t on, uint32_t _delay)
{
	if(_delay && !on)
		delay(_delay);

	if (on)
		omgpio_clear_bit(OMAP_EHCI_PHY1_RESET_GPIO);
	else
		omgpio_set_bit(OMAP_EHCI_PHY1_RESET_GPIO);
	omgpio_set_dir(OMAP_EHCI_PHY1_RESET_GPIO, OMGPIO_DIR_OUT);
	if (on)
		omgpio_clear_bit(OMAP_EHCI_PHY2_RESET_GPIO);
	else
		omgpio_set_bit(OMAP_EHCI_PHY2_RESET_GPIO);
	omgpio_set_dir(OMAP_EHCI_PHY2_RESET_GPIO, OMGPIO_DIR_OUT);

	if(_delay && !on)
		delay(_delay);

}


void omehci_uhh_init(struct omehci_softc *sc)
{

	unsigned int reg;

	reg = bus_space_read_4(sc->sc.iot, sc->hs_ioh, OMAP_UHH_HOSTCONFIG);

	/* setup ULPI bypass and burst configurations */
	reg |= (UHH_HOSTCONFIG_ENA_INCR4
		| UHH_HOSTCONFIG_ENA_INCR8
		| UHH_HOSTCONFIG_ENA_INCR16);
	reg |= UHH_HOSTCONFIG_APP_START_CLK;
	reg &= ~UHH_HOSTCONFIG_ENA_INCR_ALIGN;

	/* Clear port mode fields for PHY mode*/
	reg &= ~UHH_HOSTCONFIG_P1_MODE_MASK;
	reg &= ~UHH_HOSTCONFIG_P2_MODE_MASK;

	bus_space_write_4(sc->sc.iot, sc->hs_ioh, OMAP_UHH_HOSTCONFIG, reg);
}

int
omehci_init(struct omehci_softc *sc)
{
	uint32_t rev = 0;
	uint32_t reg;
	uint32_t i = 0;

	prcm_hsusbhost_deactivate(USBHSHOST_CLK);
	delay(1000);
	prcm_hsusbhost_activate(USBHSHOST_CLK);
	delay(1000);
	prcm_hsusbhost_set_source(USBP1_PHY_CLK, EXT_CLK);
	prcm_hsusbhost_activate(USBP1_PHY_CLK);
	prcm_hsusbhost_set_source(USBP2_PHY_CLK, EXT_CLK);
	prcm_hsusbhost_activate(USBP2_PHY_CLK);

	//omehci_phy_reset(1, 10);

	reg = bus_space_read_4(sc->sc.iot, sc->hs_ioh, OMAP_UHH_SYSCONFIG);
	reg &= ~UHH_SYSCONFIG_IDLEMODE_MASK;
	reg |=  UHH_SYSCONFIG_IDLEMODE_NOIDLE;
	reg &= ~UHH_SYSCONFIG_STANDBYMODE_MASK;
	reg |=  UHH_SYSCONFIG_STANDBYMODE_NOSTDBY;
	bus_space_write_4(sc->sc.iot, sc->hs_ioh, OMAP_UHH_SYSCONFIG, reg);

	//printf("UHH_SYSCONFIG 0x%08x\n", reg);
	rev = bus_space_read_4(sc->sc.iot, sc->hs_ioh, OMAP_UHH_REVISION);

	if (rev != OMAP_EHCI_REV2)
		return EINVAL;

	//printf("OMAP UHH_REVISION 0x%08x\n", rev);

	omehci_uhh_init(sc);

	//omehci_phy_reset(0, 10);

	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OMAP_USBHOST_INSNREG04, OMAP_USBHOST_INSNREG04_DISABLE_UNSUSPEND);

	for (i = 0; i < OMAP_HS_USB_PORTS; i++)
		if (port_mode[i] == EHCI_HCD_OMAP_MODE_PHY)
			omehci_soft_phy_reset(sc, i);

	return(0);
}

void
omehci_soft_phy_reset(struct omehci_softc *sc, unsigned int port)
{
	unsigned long timeout = (hz < 10) ? 1 : ((100 * hz) / 1000);
	uint32_t reg;

	reg = ULPI_FUNC_CTRL_RESET
		/* FUNCTION_CTRL_SET register */
		| (ULPI_SET(ULPI_FUNC_CTRL) << OMAP_USBHOST_INSNREG05_ULPI_REGADD_SHIFT)
		/* Write */
		| (2 << OMAP_USBHOST_INSNREG05_ULPI_OPSEL_SHIFT)
		/* PORTn */
		| ((port + 1) << OMAP_USBHOST_INSNREG05_ULPI_PORTSEL_SHIFT)
		/* start ULPI access*/
		| (1 << OMAP_USBHOST_INSNREG05_ULPI_CONTROL_SHIFT);

	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OMAP_USBHOST_INSNREG05_ULPI, reg);

	timeout += 1000000;
	/* Wait for ULPI access completion */
	while ((bus_space_read_4(sc->sc.iot, sc->sc.ioh, OMAP_USBHOST_INSNREG05_ULPI)
			& (1 << OMAP_USBHOST_INSNREG05_ULPI_CONTROL_SHIFT))) {

		/* Sleep for a tick */
		delay(10);

		if (timeout-- == 0) {
			printf("PHY reset operation timed out\n");
			break;
		}
	}
}


int
omehci_detach(struct device *self, int flags)
{
	struct omehci_softc		*sc = (struct omehci_softc *)self;
	int				rv;

	rv = ehci_detach(&sc->sc, flags);
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

	/* stop clock */
#if 0
	prcm_disableclock(PRCM_CLK_EN_USB);
#endif

	return (0);
}

int
omehci_activate(struct device *self, int act)
{
	struct omehci_softc *sc = (struct omehci_softc *)self;

	switch (act) {
	case DVACT_SUSPEND:
		sc->sc.sc_bus.use_polling++;
#if 0
		ohci_activate(&sc->sc, act);
		prcm_disableclock(PRCM_CLK_EN_USB);
#endif
		sc->sc.sc_bus.use_polling--;
		break;
	case DVACT_RESUME:
		sc->sc.sc_bus.use_polling++;
//		prcm_enableclock(PRCM_CLK_EN_USB);
#if 0
		omehci_enable(sc);
		ohci_activate(&sc->sc, act);
#endif
		sc->sc.sc_bus.use_polling--;
		break;
	case DVACT_POWERDOWN:
		ehci_reset(sc->sc);
		break;
	}
	return 0;
}

#if 0
void
omehci_enable(struct omehci_softc *sc)
{
	u_int32_t			hr;

	/* Full host reset */
	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) | USBHC_HR_FHR);

	DELAY(USBHC_RST_WAIT);

	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) & ~(USBHC_HR_FHR));

	/* Force system bus interface reset */
	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) | USBHC_HR_FSBIR);

	while (bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR) & \
	    USBHC_HR_FSBIR)
		DELAY(3);

	/* Enable the ports (physically only one, only enable that one?) */
	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) & ~(USBHC_HR_SSE));
	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) & ~(USBHC_HR_SSEP2));
}

void
omehci_disable(struct omehci_softc *sc)
{
	u_int32_t			hr;

	/* Full host reset */
	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) | USBHC_HR_FHR);

	DELAY(USBHC_RST_WAIT);

	hr = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBHC_HR);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBHC_HR,
	    (hr & USBHC_HR_MASK) & ~(USBHC_HR_FHR));
}
#endif
