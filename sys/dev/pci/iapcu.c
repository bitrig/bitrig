/*
 * Copyright (c) 2015 genua mbh. All rights reserved.
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
 *
 * Written by Pedro Martelletto <pedro@ambientworks.net>.
 */

/*
 * Intel Atom C2000 Platform Controller Unit (PCU) driver. The PCU is an
 * abstraction encompassing multiple hardware blocks, some of which expose
 * functionalities already dealt with by acpi(4), such as a High Precision
 * Event Timer (HPET) or a Real Time Clock (RTC). The remaining blocks, while
 * not entirely orthogonal to ACPI, implement features that would otherwise
 * remain unhandled by the kernel, such as access to GPIO pins and a watchdog
 * timer. This driver implements both of these features.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/gpio/gpiovar.h>

#define ACPI_BASE_ADDR		0x40		/* in PCI configuration space */
#define PMC_BASE_ADDR		0x44		/* in PCI configuration space */
#define GPIO_BASE_ADDR		0x48		/* in PCI configuration space */

#define GPIO_BASE_ADDR_MASK	0x0000ff00	/* in I/O space */
#define PMC_BASE_ADDR_MASK	0xfffffe00	/* in memory space */
#define ACPI_BASE_ADDR_MASK	0x0000ff80	/* in I/O space */

#define TCO_REG_OFFSET		0x60		/* in bytes */

#define GPIO_SPACE_SIZE		256		/* in bytes */
#define PMC_SPACE_SIZE		512		/* in bytes */
#define TCO_SPACE_SIZE		16		/* in bytes */

#define GPIO_USE_SEL		0x00		/* active pins */
#define GPIO_IO_SEL		0x04		/* i/o direction */
#define GPIO_GP_LVL		0x08		/* pin levels */

#define PMC_PRSTS		0x00		/* power and reset status */
#define PMC_CFG			0x08		/* pmc configuration */
#define PMC_CFG_NO_REBOOT	0x10		/* if set, tco doesn't reboot */

#define TCO_RLD			0x00		/* triggers watchdog reload */
#define TCO_STS			0x04		/* watchdog status */
#define TCO1_CNT		0x08		/* watchdog control flags */
#define TCO_TMR			0x10		/* watchdog value */

#define TCO1_CNT_LOCK		0x01000		/* watchdog locked */
#define TCO1_CNT_HALT		0x00800		/* watchdog halted */

/*
 * The SoC offers 59 GPIO pins, 31 connected to the core power well and 28 to
 * the suspend power well. We currently only support the former set.
 */
#define GPIO_NPINS	31

struct iapcu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;

	bus_space_handle_t	sc_tco_ioh;
	bus_space_handle_t	sc_gpio_ioh;

	int			sc_wdog_period;

	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIO_NPINS];
	/*
	 * We need to keep a copy of GPIO_GP_LVL for output-only pins, since
	 * their status can't be retrieved by reading in the register.
	 */
	uint32_t		sc_gp_lvl;
};

void	pcibattach(struct device *, struct device *, void *);
void	iapcu_attach(struct device *, struct device *, void *);
void	iapcu_attach_gpio(struct iapcu_softc *, struct pci_attach_args *);
void	iapcu_attach_wdog(struct iapcu_softc *, struct pci_attach_args *);
void	iapcu_wdog_start(struct iapcu_softc *);
void	iapcu_wdog_stop(struct iapcu_softc *);
void	iapcu_wdog_set(struct iapcu_softc *, int);
void	iapcu_wdog_reload(struct iapcu_softc *);
void	iapcu_gpio_pin_write(void *, int, int);
void	iapcu_gpio_pin_ctl(void *, int, int);

int	iapcu_match(struct device *, void *, void *);
int	iapcu_wdog_cb(void *, int);
int	iapcu_gpio_pin_read(void *, int);

struct cfdriver iapcu_cd = {
	NULL, "iapcu", DV_DULL
};

struct cfattach iapcu_ca = {
	sizeof(struct iapcu_softc), iapcu_match, iapcu_attach,
};

const struct pci_matchid iapcu_devices[] = {
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_ATOMC2000_PCU_1 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_ATOMC2000_PCU_2 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_ATOMC2000_PCU_3 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_ATOMC2000_PCU_4 },
};

int
iapcu_match(struct device *parent, void *match, void *aux)
{
	if (pci_matchbyid((struct pci_attach_args *)aux, iapcu_devices,
	    sizeof(iapcu_devices) / sizeof(iapcu_devices[0])))
		return (2);

	return (0);
}

int
iapcu_gpio_pin_read(void *arg, int pin)
{
	struct iapcu_softc *sc = arg;
	uint32_t gp_lvl;
	int status;

	gp_lvl = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_GP_LVL);
	if (gp_lvl & (1 << pin)) {
		status = GPIO_PIN_HIGH;
		sc->sc_gp_lvl |= (1 << pin);
	} else {
		status = GPIO_PIN_LOW;
		sc->sc_gp_lvl &= ~(1 << pin);
	}

	return (status);
}

void
iapcu_gpio_pin_write(void *arg, int pin, int value)
{
	struct iapcu_softc *sc = arg;

	if (value == GPIO_PIN_HIGH)
		sc->sc_gp_lvl |= 1 << pin;
	else
		sc->sc_gp_lvl &= ~(1 << pin);

	bus_space_write_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_GP_LVL,
	    sc->sc_gp_lvl);
}

/*
 * We can't return an error from this function. :-( There are two error cases
 * we need to handle here, though: the request pin is not available, or it is
 * available only in a direction other than the one requested.
 */
void
iapcu_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct iapcu_softc *sc = (struct iapcu_softc *)arg;
	uint32_t use_sel, io_sel;

	use_sel = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_USE_SEL);
	if (pin > GPIO_NPINS || (use_sel & (1 << pin)) == 0) {
		printf("%s: pin %d unavailable\n", __func__, pin);
		return;
	}

	io_sel = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_IO_SEL);
	if (flags & GPIO_PIN_INPUT)
		io_sel |= 1 << pin;
	else
		io_sel &= ~(1 << pin);

	bus_space_write_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_IO_SEL, io_sel);
	bus_space_barrier(sc->sc_iot, sc->sc_gpio_ioh, 0, GPIO_SPACE_SIZE,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	io_sel = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_IO_SEL);
	if (flags & GPIO_PIN_INPUT) {
		if ((io_sel & (1 << pin)) == 0)
			printf("%s: pin %d is output only\n", __func__, pin);
		else
			sc->sc_gpio_pins[pin].pin_state =
				iapcu_gpio_pin_read(arg, pin);
	} else {
		if ((io_sel & (1 << pin)) != 0)
			printf("%s: pin %d is input only\n", __func__, pin);
		sc->sc_gpio_pins[pin].pin_state = 0;	/* XXX */
	}
}

void
iapcu_attach_gpio(struct iapcu_softc *sc, struct pci_attach_args *pa)
{
	struct gpiobus_attach_args gba;
	uint32_t gbase;
	int i;

	gbase = pci_conf_read(pa->pa_pc, pa->pa_tag, GPIO_BASE_ADDR);
	gbase &= GPIO_BASE_ADDR_MASK;

	if (bus_space_map(sc->sc_iot, gbase, GPIO_SPACE_SIZE, 0,
	    &sc->sc_gpio_ioh) != 0) {
		printf("%s: couldn't map gpio space\n", sc->sc_dev.dv_xname);
		return;
	}

	for (i = 0; i < GPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = iapcu_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = iapcu_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = iapcu_gpio_pin_ctl;

	gba.gba_name = "gpio";
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = GPIO_NPINS;

	(void)config_found(&sc->sc_dev, &gba, gpiobus_print);
}

void
iapcu_wdog_start(struct iapcu_softc *sc)
{
	uint32_t tco1_cnt;

	tco1_cnt = bus_space_read_4(sc->sc_iot, sc->sc_tco_ioh, TCO1_CNT);
	tco1_cnt &= ~TCO1_CNT_HALT;
	bus_space_write_4(sc->sc_iot, sc->sc_tco_ioh, TCO1_CNT, tco1_cnt);
}

void
iapcu_wdog_stop(struct iapcu_softc *sc)
{
	uint32_t tco1_cnt;

	tco1_cnt = bus_space_read_4(sc->sc_iot, sc->sc_tco_ioh, TCO1_CNT);
	tco1_cnt |= TCO1_CNT_HALT;
	bus_space_write_4(sc->sc_iot, sc->sc_tco_ioh, TCO1_CNT, tco1_cnt);
}

void
iapcu_wdog_set(struct iapcu_softc *sc, int period)
{
	uint32_t tco_tmr, value;

	value = (period << 16);
	tco_tmr = bus_space_read_4(sc->sc_iot, sc->sc_tco_ioh, TCO_TMR);
	tco_tmr |= value;

	bus_space_write_4(sc->sc_iot, sc->sc_tco_ioh, TCO_TMR, tco_tmr);
	bus_space_write_4(sc->sc_iot, sc->sc_tco_ioh, TCO_RLD, 0);

	iapcu_wdog_start(sc);
}

void
iapcu_wdog_reload(struct iapcu_softc *sc)
{
	/* issued periodically to prevent a timeout */
	bus_space_write_4(sc->sc_iot, sc->sc_tco_ioh, TCO_RLD, 0);
}

int
iapcu_wdog_cb(void *arg, int period)
{
	struct iapcu_softc *sc = arg;

	if (period == 0) {
		if (sc->sc_wdog_period != 0)
			iapcu_wdog_stop(sc);
	} else {
		if (period > 600)
			period = 600;	/* maximum supported timeout */
		if (sc->sc_wdog_period != period)
			iapcu_wdog_set(sc, period);
		if (sc->sc_wdog_period == 0)
			iapcu_wdog_start(sc);
		else
			iapcu_wdog_reload(sc);
	}

	sc->sc_wdog_period = period;

	return (period);
}

void
iapcu_attach_wdog(struct iapcu_softc *sc, struct pci_attach_args *pa)
{
	uint32_t abase, pbase, pmc_cfg;
	bus_space_handle_t pmc_ioh;

	/*
	 * Locate 'abase' and 'pbase' in PCI configuration space.
	 */

	abase = pci_conf_read(pa->pa_pc, pa->pa_tag, ACPI_BASE_ADDR);
	abase &= ACPI_BASE_ADDR_MASK;

	pbase = pci_conf_read(pa->pa_pc, pa->pa_tag, PMC_BASE_ADDR);
	pbase &= PMC_BASE_ADDR_MASK;

	/*
	 * Map TCO registers, a subset of ACPI's.
	 */

	if (bus_space_map(sc->sc_iot, abase + TCO_REG_OFFSET, TCO_SPACE_SIZE, 0,
	    &sc->sc_tco_ioh) != 0) {
		printf("%s: couldn't map tco space\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Map a single PMC register (PMC_CFG), needed to enable watchdog
	 * reboots.
	 */

	if (bus_space_map(pa->pa_memt, pbase, PMC_SPACE_SIZE, 0,
	    &pmc_ioh) != 0) {
		printf("%s: couldn't map pmc space\n", sc->sc_dev.dv_xname);
		goto unmap;
	}

	pmc_cfg = bus_space_read_4(pa->pa_memt, pmc_ioh, PMC_CFG);
	pmc_cfg &= ~PMC_CFG_NO_REBOOT; /* Activate watchdog reboots. */

	bus_space_write_4(pa->pa_memt, pmc_ioh, PMC_CFG, pmc_cfg);
	bus_space_barrier(pa->pa_memt, pmc_ioh, 0, PMC_SPACE_SIZE,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	pmc_cfg = bus_space_read_4(pa->pa_memt, pmc_ioh, PMC_CFG);
	bus_space_unmap(pa->pa_memt, pmc_ioh, PMC_SPACE_SIZE);
	if (pmc_cfg & PMC_CFG_NO_REBOOT) {
		printf("%s: watchdog can't reboot\n", sc->sc_dev.dv_xname);
		goto unmap;
	}

	/*
	 * XXX Apparently there's no way to detect if a reboot was caused by a
	 * watchdog timeout?
	 */

	iapcu_wdog_stop(sc);
	sc->sc_wdog_period = 0;
	wdog_register(iapcu_wdog_cb, sc);

	printf("%s: watchdog available\n", sc->sc_dev.dv_xname);

	return;

unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_tco_ioh, TCO_SPACE_SIZE);
}

void
iapcu_attach(struct device *parent, struct device *self, void *aux)
{
	struct iapcu_softc *sc = (struct iapcu_softc *)self;
	struct pci_attach_args *pa = aux;

	sc->sc_iot = pa->pa_iot;

	pcibattach(parent, self, aux);
	iapcu_attach_wdog(sc, pa);
	iapcu_attach_gpio(sc, pa);
}
