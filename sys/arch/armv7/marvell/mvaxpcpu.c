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
#include <sys/sensors.h>
#include <sys/sysctl.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/marvell/mvpmsuvar.h>

#define CPU_DIVCLK_CTRL0			0x0
#define  CPU_DIVCLK_CTRL0_RESET_MASK			0xff
#define  CPU_DIVCLK_CTRL0_RESET_SHIFT			8
#define CPU_DIVCLK_CTRL2			0x8
#define  CPU_DIVCLK_CTRL2_NBCLK_MASK			0x3f
#define  CPU_DIVCLK_CTRL2_NBCLK_SHIFT			16
#define CPU_DIVCLK_CTRL3			0xC
#define  CPU_DIVCLK_CTRL3_CLKDIV_DIV_MASK		0x3f
#define  CPU_DIVCLK_CTRL3_CLKDIV_DIV_SHIFT(cpu)		((cpu) * 8)
#define  CPU_DIVCLK_CTRL3_CLKDIV_DIV(x, cpu)		\
		(((x) >> CPU_DIVCLK_CTRL3_CLKDIV_DIV_SHIFT(cpu)) & \
		    CPU_DIVCLK_CTRL3_CLKDIV_DIV_MASK)

#define PMU_DFS_CTRL(cpu)			(0x4 * (cpu))
#define  PMU_DFS_CTRL_REQUEST_DIV_SHIFT			16
#define  PMU_DFS_CTRL_REQUEST_DIV_MASK			0x3f

#define HREAD4(sc, p, reg)						\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ ## p ## _ioh, (reg)))
#define HWRITE4(sc, p, reg, val)					\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ ## p ## _ioh, (reg), (val))
#define HSET4(sc, p, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), p, (reg)) | (bits))
#define HCLR4(sc, p, reg, bits)						\
	HWRITE4((sc), p, (reg), HREAD4((sc), p, (reg)) & ~(bits))

struct mvaxpcpu_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_cpu_ioh;
	bus_space_handle_t	 sc_pmu_ioh;
	struct clk		*sc_clk;

	struct ksensor		 sc_freq_sensor[4];
	struct ksensordev	 sc_sensordev;
};

struct mvaxpcpu_softc *mvaxpcpu_sc;

int	 mvaxpcpu_match(struct device *, void *, void *);
void	 mvaxpcpu_attach(struct device *, struct device *, void *);
uint32_t mvaxpcpu_get_rate(struct mvaxpcpu_softc *, int);
void	 mvaxpcpu_on_set_rate(struct mvaxpcpu_softc *, int, uint32_t);
void	 mvaxpcpu_setperf(int);
void	 mvaxpcpu_refresh_sensors(void *);

struct cfattach	mvaxpcpu_ca = {
	sizeof (struct mvaxpcpu_softc), mvaxpcpu_match, mvaxpcpu_attach
};

struct cfdriver mvaxpcpu_cd = {
	NULL, "mvaxpcpu", DV_DULL
};

int
mvaxpcpu_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armada-xp-cpu-clock", aa->aa_node))
		return (1);

	return 0;
}

void
mvaxpcpu_attach(struct device *parent, struct device *self, void *args)
{
	struct mvaxpcpu_softc *sc = (struct mvaxpcpu_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;

	printf("\n");

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_cpu_ioh))
		panic("%s: bus_space_map failed!", __func__);

	if (fdt_get_memory_address(aa->aa_node, 1, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_pmu_ioh))
		panic("%s: bus_space_map failed!", __func__);

	sc->sc_clk = clk_fdt_get(aa->aa_node, 0);
	if (sc->sc_clk == NULL)
		panic("%s: could not extract clock from FDT", __func__);

	mvaxpcpu_sc = sc;
	cpu_setperf = mvaxpcpu_setperf;

	/* setup CPU sensors */
	strlcpy(sc->sc_sensordev.xname, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensordev.xname));

	sc->sc_freq_sensor[0].type = SENSOR_FREQ;
	sensor_attach(&sc->sc_sensordev, &sc->sc_freq_sensor[0]);
	sc->sc_freq_sensor[1].type = SENSOR_FREQ;
	sensor_attach(&sc->sc_sensordev, &sc->sc_freq_sensor[1]);
	sc->sc_freq_sensor[2].type = SENSOR_FREQ;
	sensor_attach(&sc->sc_sensordev, &sc->sc_freq_sensor[2]);
	sc->sc_freq_sensor[3].type = SENSOR_FREQ;
	sensor_attach(&sc->sc_sensordev, &sc->sc_freq_sensor[3]);

	sensordev_install(&sc->sc_sensordev);
	sensor_task_register(sc, mvaxpcpu_refresh_sensors, 60);
}

uint32_t
mvaxpcpu_get_rate(struct mvaxpcpu_softc *sc, int cpu)
{
	uint32_t reg = HREAD4(sc, cpu, CPU_DIVCLK_CTRL3);
	uint32_t div = CPU_DIVCLK_CTRL3_CLKDIV_DIV(reg, cpu);
	return clk_get_rate(sc->sc_clk) / div;
}

void
mvaxpcpu_on_set_rate(struct mvaxpcpu_softc *sc, int cpu, uint32_t rate)
{
	uint32_t cur_rate, cpu_div, nbclk_div, reg;

	/* PMSU available? */
	if (!mvpmsu_dfs_available())
		return;

	/* Nothing to change? */
	cur_rate = mvaxpcpu_get_rate(sc, cpu);
	if (rate == cur_rate)
		return;

	/* CPU div depends on fabric div. */
	nbclk_div = (HREAD4(sc, cpu, CPU_DIVCLK_CTRL2) >>
	    CPU_DIVCLK_CTRL2_NBCLK_SHIFT) & CPU_DIVCLK_CTRL2_NBCLK_MASK;

	/* Scale up? */
	if (rate > cur_rate)
		cpu_div = nbclk_div / 2; /* 2 * nbclk freq */
	else
		cpu_div = nbclk_div; /* nbclk freq */

	/* Need a divisor. */
	if (cpu_div == 0)
		cpu_div = 1;

	/* Request ratio. */
	reg = HREAD4(sc, pmu, PMU_DFS_CTRL(cpu));
	reg &= ~(PMU_DFS_CTRL_REQUEST_DIV_MASK <<
	    PMU_DFS_CTRL_REQUEST_DIV_SHIFT);
	reg |= cpu_div << PMU_DFS_CTRL_REQUEST_DIV_SHIFT;
	HWRITE4(sc, pmu, PMU_DFS_CTRL(cpu), reg);

	/* Set all. */
	HWRITE4(sc, cpu, CPU_DIVCLK_CTRL0,
	    CPU_DIVCLK_CTRL0_RESET_MASK << CPU_DIVCLK_CTRL0_RESET_SHIFT);

	/* Go! */
	mvpmsu_dfs_request(cpu);
}

void
mvaxpcpu_setperf(int level)
{
	struct mvaxpcpu_softc *sc = mvaxpcpu_sc;

	if (level > 50)
		mvaxpcpu_on_set_rate(sc, 0, clk_get_rate(sc->sc_clk));
	else
		mvaxpcpu_on_set_rate(sc, 0, clk_get_rate(sc->sc_clk) / 2);
}

void
mvaxpcpu_refresh_sensors(void *arg)
{
	struct mvaxpcpu_softc *sc = (struct mvaxpcpu_softc *)arg;

	strlcpy(sc->sc_freq_sensor[0].desc, "cpu0",
	    sizeof(sc->sc_freq_sensor[0].desc));
	sc->sc_freq_sensor[0].value =
	    mvaxpcpu_get_rate(sc, 0) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[1].desc, "cpu1",
	    sizeof(sc->sc_freq_sensor[1].desc));
	sc->sc_freq_sensor[1].value =
	    mvaxpcpu_get_rate(sc, 1) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[2].desc, "cpu2",
	    sizeof(sc->sc_freq_sensor[2].desc));
	sc->sc_freq_sensor[2].value =
	    mvaxpcpu_get_rate(sc, 2) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[3].desc, "cpu3",
	    sizeof(sc->sc_freq_sensor[3].desc));
	sc->sc_freq_sensor[3].value =
	    mvaxpcpu_get_rate(sc, 3) * 1000LL * 1000LL;
}
