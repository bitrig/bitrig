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
#include <armv7/armv7/armv7var.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include "gpio.h"

#define GPIOFSEL(x)		(0x00 + (x) * 4)
#define GPIOSET(x)		(0x1c + (x) * 4)
#define GPIOCLR(x)		(0x28 + (x) * 4)
#define GPIOLEV(x)		(0x34 + (x) * 4)
#define GPIOEDS(x)		(0x40 + (x) * 4)
#define GPIOREN(x)		(0x4c + (x) * 4)
#define GPIOFEN(x)		(0x58 + (x) * 4)
#define GPIOHEN(x)		(0x64 + (x) * 4)
#define GPIOLEN(x)		(0x70 + (x) * 4)
#define GPIOAREN(x)		(0x7c + (x) * 4)
#define GPIOAFREN(x)		(0x88 + (x) * 4)
#define GPIOUD(x)		(0x94 + (x) * 4)
#define GPIOUDCLK(x)		(0x98 + (x) * 4)

#define GPIO_FSEL_INPUT		0
#define GPIO_FSEL_OUTPUT	1
#define GPIO_FSEL_ALT5		2
#define GPIO_FSEL_ALT4		3
#define GPIO_FSEL_ALT0		4
#define GPIO_FSEL_ALT1		5
#define GPIO_FSEL_ALT2		6
#define GPIO_FSEL_ALT3		7

#define GPIO_NPINS		54
#define GPIO_NBANKS		2

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct bcmgpio_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	struct gpio_chipset_tag	 sc_gpio_gc;
	gpio_pin_t		 sc_gpio_pins[GPIO_NPINS];
};

int	 bcmgpio_match(struct device *, void *, void *);
void	 bcmgpio_attach(struct device *, struct device *, void *);
int	 bcmgpio_pin_read(void *, int);
void	 bcmgpio_pin_write(void *, int, int);
void	 bcmgpio_pin_ctl(void *, int, int);
int	 bcmgpio_func_read(struct bcmgpio_softc *, int);
void	 bcmgpio_func_set(struct bcmgpio_softc *, int, int);

struct cfattach	bcmgpio_ca = {
	sizeof (struct bcmgpio_softc), bcmgpio_match, bcmgpio_attach
};

struct cfdriver bcmgpio_cd = {
	NULL, "bcmgpio", DV_DULL
};

int
bcmgpio_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("brcm,bcm2835-gpio", aa->aa_node) ||
	    fdt_node_compatible("broadcom,bcm2835-gpio", aa->aa_node))
		return (1);

	return 0;
}

void
bcmgpio_attach(struct device *parent, struct device *self, void *args)
{
	struct bcmgpio_softc *sc = (struct bcmgpio_softc *)self;
	struct armv7_attach_args *aa = args;
	struct gpiobus_attach_args gba;
	struct fdt_memory mem;
	int i;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	for (i = 0; i < GPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;

		int func = bcmgpio_func_read(sc, i);
		if (func == GPIO_FSEL_INPUT || func == GPIO_FSEL_OUTPUT) {
			sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
			    GPIO_PIN_OUTPUT |
			    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
			    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
			sc->sc_gpio_pins[i].pin_state =
			    bcmgpio_pin_read(sc, i);
		} else {
			sc->sc_gpio_pins[i].pin_caps = 0;
			sc->sc_gpio_pins[i].pin_state = 0;
		}
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = bcmgpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = bcmgpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = bcmgpio_pin_ctl;

	gba.gba_name = "gpio";
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = GPIO_NPINS;

	config_found(&sc->sc_dev, &gba, gpiobus_print);
}

int
bcmgpio_pin_read(void *arg, int pin)
{
	struct bcmgpio_softc *sc = arg;
	int bank = pin / 32;
	int off = pin % 32;
	int reg;

	if (pin >= GPIO_NPINS)
		return 0;

	reg = HREAD4(sc, GPIOLEV(bank));

	return !!(reg & (1U << off));
}

void
bcmgpio_pin_write(void *arg, int pin, int val)
{
	struct bcmgpio_softc *sc = arg;
	int bank = pin / 32;
	int off = pin % 32;

	if (pin >= GPIO_NPINS)
		return;

	if (val)
		HWRITE4(sc, GPIOSET(bank), 1U << off);
	else
		HWRITE4(sc, GPIOCLR(bank), 1U << off);

	return;
}

void
bcmgpio_pin_ctl(void *arg, int pin, int flags)
{
	struct bcmgpio_softc *sc = arg;
	int bank = pin / 32;
	int off = pin % 32;

	if (pin >= GPIO_NPINS)
		return;

	if (flags & GPIO_PIN_INPUT)
		bcmgpio_func_set(sc, pin, GPIO_FSEL_INPUT);
	else if (flags & GPIO_PIN_OUTPUT)
		bcmgpio_func_set(sc, pin, GPIO_FSEL_OUTPUT);

	if (flags & GPIO_PIN_PULLUP)
		HWRITE4(sc, GPIOUD(0), 2);
	else if (flags & GPIO_PIN_PULLDOWN)
		HWRITE4(sc, GPIOUD(0), 1);
	else
		HWRITE4(sc, GPIOUD(0), 0);

	delay(5);
	HWRITE4(sc, GPIOUDCLK(bank), 1 << off);
	delay(5);
	HWRITE4(sc, GPIOUD(0), 0);
	HWRITE4(sc, GPIOUDCLK(bank), 0 << off);

	return;
}

int
bcmgpio_func_read(struct bcmgpio_softc *sc, int pin)
{
	int bank = pin / 10;
	int shift = (pin % 10) * 3;
	uint32_t reg;

	reg = HREAD4(sc, GPIOFSEL(bank));

	return ((reg >> shift) & 0x7);
}


void
bcmgpio_func_set(struct bcmgpio_softc *sc, int pin, int func)
{
	int bank = pin / 10;
	int shift = (pin % 10) * 3;
	uint32_t reg;

	reg = HREAD4(sc, GPIOFSEL(bank));
	reg &= ~(0x7 << shift);
	reg |= func << shift;
	HWRITE4(sc, GPIOFSEL(bank), reg);
}
