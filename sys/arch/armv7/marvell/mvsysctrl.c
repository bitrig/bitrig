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
#include <sys/device.h>
#include <sys/sysctl.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct mvsysctrl_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
};

int	 mvsysctrl_match(struct device *, void *, void *);
void	 mvsysctrl_attach(struct device *, struct device *, void *);

void	 mvsysctrl_reset(void);

struct mvsysctrl_softc *mvsysctrl_sc;

struct cfattach	mvsysctrl_ca = {
	sizeof (struct mvsysctrl_softc), mvsysctrl_match, mvsysctrl_attach
};

struct cfdriver mvsysctrl_cd = {
	NULL, "mvsysctrl", DV_DULL
};

int
mvsysctrl_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-370-xp-system-controller",
	    aa->aa_node))
		return (1);

	return 0;
}

void
mvsysctrl_attach(struct device *parent, struct device *self, void *args)
{
	struct mvsysctrl_softc *sc = (struct mvsysctrl_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	mvsysctrl_sc = sc;

	extern void (*fdt_platform_watchdog_reset_fn)(void);
	fdt_platform_watchdog_reset_fn = mvsysctrl_reset;
}

void
mvsysctrl_reset(void)
{
	struct mvsysctrl_softc *sc = mvsysctrl_sc;
	HWRITE4(sc, 0x60, 1);
	HWRITE4(sc, 0x64, 1);
	while (1)
		;
}
