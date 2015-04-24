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
#include <sys/sensors.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

#define THERM_SENSOR_CONFIG0			0x0
#define 	THERM_SENSOR_CONFIG0_SOFT_RESET		(1 << 1)
#define 	THERM_SENSOR_CONFIG0_REF_CAL_CNT_MASK	0x1ff
#define 	THERM_SENSOR_CONFIG0_REF_CAL_CNT_SHIFT	11
#define 	THERM_SENSOR_CONFIG0_REF_CAL_CNT(x)	(((x) & THERM_SENSOR_CONFIG0_REF_CAL_CNT_MASK) \
							    << THERM_SENSOR_CONFIG0_REF_CAL_CNT_SHIFT)
#define 	THERM_SENSOR_CONFIG0_OTF		(1 << 30)

#define THERM_SENSOR_STATUS			0x0
#define 	THERM_SENSOR_STATUS_ENABLE		(1 << 0)
#define 	THERM_SENSOR_TEMP_OUT_MASK		0x1ff
#define 	THERM_SENSOR_TEMP_OUT_SHIFT		10
#define 	THERM_SENSOR_TEMP_OUT(x)		(((x) >> THERM_SENSOR_TEMP_OUT_SHIFT) \
							    & THERM_SENSOR_TEMP_OUT_MASK)

struct mvaxptherm_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_sioh;
	bus_space_handle_t	 sc_cioh;

	struct ksensor		 sc_sensor;
	struct ksensordev	 sc_sensordev;
};

int	 mvaxptherm_match(struct device *, void *, void *);
void	 mvaxptherm_attach(struct device *, struct device *, void *);
void	 mvaxptherm_refresh_sensors(void *);

struct cfattach	mvaxptherm_ca = {
	sizeof (struct mvaxptherm_softc), mvaxptherm_match, mvaxptherm_attach
};

struct cfdriver mvaxptherm_cd = {
	NULL, "mvaxptherm", DV_DULL
};

int
mvaxptherm_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,armadaxp-thermal", aa->aa_node))
		return (1);

	return 0;
}

void
mvaxptherm_attach(struct device *parent, struct device *self, void *args)
{
	struct mvaxptherm_softc *sc = (struct mvaxptherm_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	uint32_t reg;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_sioh))
		panic("%s: bus_space_map failed!", __func__);

	if (fdt_get_memory_address(aa->aa_node, 1, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_cioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	reg = bus_space_read_4(sc->sc_iot, sc->sc_cioh, THERM_SENSOR_CONFIG0);

	/* Enable OTF. */
	reg |= THERM_SENSOR_CONFIG0_OTF;
	bus_space_write_4(sc->sc_iot, sc->sc_cioh, THERM_SENSOR_CONFIG0, reg);

	/* Set reference calibration value. */
	reg &= ~(THERM_SENSOR_CONFIG0_REF_CAL_CNT(THERM_SENSOR_CONFIG0_REF_CAL_CNT_MASK));
	reg |= THERM_SENSOR_CONFIG0_REF_CAL_CNT(0xf1);
	bus_space_write_4(sc->sc_iot, sc->sc_cioh, THERM_SENSOR_CONFIG0, reg);

	/* Reset. */
	reg |= THERM_SENSOR_CONFIG0_SOFT_RESET;
	bus_space_write_4(sc->sc_iot, sc->sc_cioh, THERM_SENSOR_CONFIG0, reg);

	/* Release. */
	reg &= ~THERM_SENSOR_CONFIG0_SOFT_RESET;
	bus_space_write_4(sc->sc_iot, sc->sc_cioh, THERM_SENSOR_CONFIG0, reg);

	/* Enable. */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_sioh, THERM_SENSOR_STATUS);
	reg |= THERM_SENSOR_STATUS_ENABLE;
	bus_space_write_4(sc->sc_iot, sc->sc_sioh, THERM_SENSOR_STATUS, reg);

	strlcpy(sc->sc_sensordev.xname, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensordev.xname));
	strlcpy(sc->sc_sensor.desc, "core",
	    sizeof(sc->sc_sensor.desc));
	sc->sc_sensor.type = SENSOR_TEMP;
	sensor_attach(&sc->sc_sensordev, &sc->sc_sensor);
	sensordev_install(&sc->sc_sensordev);
	sensor_task_register(sc, mvaxptherm_refresh_sensors, 5);
}

void
mvaxptherm_refresh_sensors(void *arg)
{
	struct mvaxptherm_softc *sc = (struct mvaxptherm_softc *)arg;
	uint64_t reg;

	reg = bus_space_read_4(sc->sc_iot, sc->sc_sioh,
	    THERM_SENSOR_STATUS);
	reg = THERM_SENSOR_TEMP_OUT(reg);
	reg = (3153000000000 - (reg * 10000000000)) / 13825;

	sc->sc_sensor.value = reg + 273150000LL;
}
