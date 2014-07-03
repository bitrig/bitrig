/* $OpenBSD$ */
/*
 * Copyright (c) 2013 Patrick Wildt <patrick@blueri.se>
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
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/evcount.h>

#include <machine/bus.h>
#include <arm/mainbus/mainbus.h>
#include <arm/cortex/cortex.h>
#include <arm/cpufunc.h>

/* offset from periphbase */
#define SCU_ADDR		0x000
#define SCU_SIZE		0x100

/* registers */
#define SCU_CTRL		0x00
#define SCU_CONF		0x04
#define SCU_CPU_PWR_STS		0x08
#define SCU_INV_ALL_REG_IN_SS	0x0C
#define SCU_FILTER_START_ADDR	0x40
#define SCU_FILTER_END_ADDR	0x44
#define SCU_ACCESS_CTRL		0x50
#define SCU_NS_ACCESS_CTRL	0x54

#define SCU_CONF_NCPU_MASK	(3 << 0)

struct ampscu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

int		 ampscu_match(struct device *, void *, void *);
void		 ampscu_attach(struct device *, struct device *, void *);
int		 ampscu_ncpus(struct ampscu_softc *);

struct cfattach ampscu_ca = {
	sizeof (struct ampscu_softc), ampscu_match, ampscu_attach
};

struct cfdriver ampscu_cd = {
	NULL, "ampscu", DV_DULL
};

int
ampscu_match(struct device *parent, void *cfdata, void *aux)
{
	if ((cpufunc_id() & CPU_ID_CORTEX_A9_MASK) == CPU_ID_CORTEX_A9)
		return (1);

	return (0);
}

void
ampscu_attach(struct device *parent, struct device *self, void *args)
{
	struct ampscu_softc *sc = (struct ampscu_softc *)self;
	struct cortex_attach_args *ca = args;

	sc->sc_iot = ca->ca_iot;

	if (bus_space_map(sc->sc_iot, ca->ca_periphbase + SCU_ADDR,
	    SCU_SIZE, 0, &sc->sc_ioh))
		panic("ampscu_attach: bus_space_map failed!");

	ncpusfound = ampscu_ncpus(sc);

	printf(": %d CPUs\n", ncpusfound);

#ifdef MULTIPROCESSOR
	/* ARM Errata 764369 */
	if ((curcpu()->ci_arm_cpuid & CPU_ID_CORTEX_A9_MASK) == CPU_ID_CORTEX_A9)
	    bus_space_write_4(sc->sc_iot, sc->sc_ioh, 0x30,
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, 0x30) | 1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SCU_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SCU_CTRL) | 1);

	/* Flush ALL the caches. */
	cpu_drain_writebuf();
	cpu_idcache_wbinv_all();
	cpu_sdcache_wbinv_all();
	cpu_drain_writebuf();
#endif
}

int
ampscu_ncpus(struct ampscu_softc *sc) {
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, SCU_CONF) & SCU_CONF_NCPU_MASK) + 1;
}
