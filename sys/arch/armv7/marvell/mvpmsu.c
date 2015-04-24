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
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/marvell/mvpmsuvar.h>

#define PMSU_CTRL_CONF(cpu)		((cpu * 0x100) + 0x104)
#define  PMSU_CTRL_CONF_DFS_REQ				(1 << 18)
#define PMSU_STATUS_MASK(cpu)		((cpu * 0x100) + 0x10c)
#define PMSU_EVENTSTATUS_MASK(cpu)	((cpu * 0x100) + 0x120)
#define  PMSU_EVENTSTATUS_MASK_DFS_DONE			(1 << 1)
#define  PMSU_EVENTSTATUS_MASK_DFS_DONE_MASK		(1 << 17)

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct mvpmsu_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
};

struct mvpmsu_softc *mvpmsu_sc;

int	 mvpmsu_match(struct device *, void *, void *);
void	 mvpmsu_attach(struct device *, struct device *, void *);
uint32_t mvpmsu_get_rate(struct mvpmsu_softc *, int);
void	 mvpmsu_on_set_rate(struct mvpmsu_softc *, int, uint32_t);
void	 mvpmsu_setperf(int);

struct cfattach	mvpmsu_ca = {
	sizeof (struct mvpmsu_softc), mvpmsu_match, mvpmsu_attach
};

struct cfdriver mvpmsu_cd = {
	NULL, "mvpmsu", DV_DULL
};

int
mvpmsu_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-370-pmsu", aa->aa_node))
		return (1);

	return 0;
}

void
mvpmsu_attach(struct device *parent, struct device *self, void *args)
{
	struct mvpmsu_softc *sc = (struct mvpmsu_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;

	printf("\n");

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	mvpmsu_sc = sc;
}

/*
 * TODO: This only works assuming we run on a single core.
 * TODO: When we want to do scaling for all cores, we need
 * TODO: to make this run on every core and wait for completion.
 */
void
mvpmsu_dfs_request(int cpu)
{
	struct mvpmsu_softc *sc = mvpmsu_sc;
	intr_state_t its;

	if (sc == NULL)
		return;

	its = intr_disable();

	/* Request actual transition. */
	HSET4(sc, PMSU_CTRL_CONF(cpu), PMSU_CTRL_CONF_DFS_REQ);

	/* Wait until we're transitioned. */
	__asm volatile("wfi");

	intr_restore(its);
}

int
mvpmsu_dfs_available(void)
{
	return (mvpmsu_sc != NULL);
}
