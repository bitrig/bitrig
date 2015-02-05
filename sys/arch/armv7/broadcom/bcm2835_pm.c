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

/* registers */
#define PM_RSTC		0x1c
#define PM_RSTS		0x20
#define PM_WDOG		0x24

/* bits and bytes */
#define PM_PASSWORD		0x5a000000
#define PM_RSTC_CONFIGMASK	0x00000030
#define PM_RSTC_FULL_RESET	0x00000020
#define PM_RSTC_RESET		0x00000102
#define PM_WDOG_TIMEMASK	0x000fffff

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct bcmpm_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
};

struct bcmpm_softc *bcmpm_sc;

int	 bcmpm_match(struct device *, void *, void *);
void	 bcmpm_attach(struct device *, struct device *, void *);
int	 bcmpm_activate(struct device *, int);
int	 bcmpm_wdog_cb(void *, int);
void	 bcmpm_wdog_reset(void);

struct cfattach	bcmpm_ca = {
	sizeof (struct bcmpm_softc), bcmpm_match, bcmpm_attach, NULL,
	bcmpm_activate
};

struct cfdriver bcmpm_cd = {
	NULL, "bcmpm", DV_DULL
};

int
bcmpm_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("brcm,bcm2835-pm", aa->aa_node) ||
	    fdt_node_compatible("broadcom,bcm2835-pm", aa->aa_node))
		return (1);

	return 0;
}

void
bcmpm_attach(struct device *parent, struct device *self, void *args)
{
	struct bcmpm_softc *sc = (struct bcmpm_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	bcmpm_sc = sc;

	extern void (*fdt_platform_watchdog_reset_fn)(void);
	fdt_platform_watchdog_reset_fn = bcmpm_wdog_reset;

	wdog_register(bcmpm_wdog_cb, sc);
}

int
bcmpm_activate(struct device *self, int act)
{
	switch (act) {
	case DVACT_POWERDOWN:
		wdog_shutdown(self);
		break;
	}

	return 0;
}

void
bcmpm_wdog_set(struct bcmpm_softc *sc, uint32_t period)
{
	uint32_t rstc, wdog;

	if (period == 0) {
		HWRITE4(sc, PM_RSTC, PM_RSTC_RESET | PM_PASSWORD);
		return;
	}

	rstc = HREAD4(sc, PM_RSTC) & PM_RSTC_CONFIGMASK;
	rstc |= PM_RSTC_FULL_RESET;
	rstc |= PM_PASSWORD;

	wdog = period & PM_WDOG_TIMEMASK;
	wdog |= PM_PASSWORD;

	HWRITE4(sc, PM_RSTC, wdog);
	HWRITE4(sc, PM_RSTC, rstc);
}

int
bcmpm_wdog_cb(void *self, int period)
{
	struct bcmpm_softc *sc = self;

	bcmpm_wdog_set(sc, period << 16);

	return period;
}

void
bcmpm_wdog_reset(void)
{
	struct bcmpm_softc *sc = bcmpm_sc;
	bcmpm_wdog_set(sc, 10);
	delay(100000);
}
