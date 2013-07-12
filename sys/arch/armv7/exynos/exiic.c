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
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <machine/bus.h>

#include <armv7/exynos/exvar.h>
#include <armv7/exynos/exgpiovar.h>
#include <armv7/exynos/exiicvar.h>

/* registers */
#define I2C_CON				0x00	/* control register */
#define I2C_STAT			0x04	/* control/status register */
#define I2C_ADD				0x08	/* address register */
#define I2C_DS				0x0C	/* transmit/receive data shift register */
#define I2C_LC				0x10	/* multi-master line control register */

/* bits and bytes */
#define I2C_CON_TXCLKVAL_MASK		(0xf << 0) /* tx clock = i2cclk / (i2ccon[3:0] + 1) */
#define I2C_CON_INTPENDING		(0x1 << 4) /* 0 = no interrupt pending/clear, 1 = pending */
#define I2C_CON_TXRX_INT		(0x1 << 5) /* enable/disable */
#define I2C_CON_TXCLKSRC_16		(0x0 << 6) /* i2clk = fpclk/16 */
#define I2C_CON_TXCLKSRC_512		(0x1 << 6) /* i2clk = fpclk/512 */
#define I2C_CON_ACK			(0x1 << 7)
#define I2C_STAT_LAST_RVCD_BIT		(0x1 << 0) /* last received bit 0 => ack, 1 => no ack */
#define I2C_STAT_ADDR_ZERO_FLAG		(0x1 << 1) /* 0 => start/stop cond. detected, 1 => received slave addr 0xb */
#define I2C_STAT_ADDR_SLAVE_ZERO_FLAG	(0x1 << 2) /* 0 => start/stop cond. detected, 1 => received slave addr matches i2cadd */
#define I2C_STAT_ARBITRATION		(0x1 << 3) /* 0 => successul, 1 => failed */
#define I2C_STAT_SERIAL_OUTPUT		(0x1 << 4) /* 0 => disable tx/rx, 1 => enable tx/rx */
#define I2C_STAT_BUSY_SIGNAL		(0x1 << 5) /* 0 => not busy / stop signal generation, 1 => busy / start signal generation */
#define I2C_STAT_MODE_SEL_SLAVE_RX	(0x0 << 6) /* slave receive mode */
#define I2C_STAT_MODE_SEL_SLAVE_TX	(0x1 << 6) /* slave transmit mode */
#define I2C_STAT_MODE_SEL_MASTER_RX	(0x2 << 6) /* master receive mode */
#define I2C_STAT_MODE_SEL_MASTER_TX	(0x3 << 6) /* master transmit */
#define I2C_ADD_SLAVE_ADDR(x)		(((x) & 0x7f) << 1)
#define I2C_DS_DATA_SHIFT(x)		(((x) & 0xff) << 0)

struct exiic_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	void			*sc_ih;
	int			unit;

	struct rwlock		sc_buslock;
	struct i2c_controller	i2c_tag;

	uint16_t		frequency;
	uint16_t		intr_status;
	uint16_t		stopped;
};

void exiic_attach(struct device *, struct device *, void *);
int exiic_detach(struct device *, int);
void exiic_setspeed(struct exiic_softc *, u_int);
int exiic_intr(void *);
int exiic_wait_intr(struct exiic_softc *, int, int);
int exiic_wait_state(struct exiic_softc *, uint32_t, uint32_t);
int exiic_start(struct exiic_softc *, int, int, void *, int);
int exiic_read(struct exiic_softc *, int, int, void *, int);
int exiic_write(struct exiic_softc *, int, int, const void *, int);

int exiic_i2c_acquire_bus(void *, int);
void exiic_i2c_release_bus(void *, int);
int exiic_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
    void *, size_t, int);

#define HREAD2(sc, reg)							\
	(bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE2(sc, reg, val)						\
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET2(sc, reg, bits)						\
	HWRITE2((sc), (reg), HREAD2((sc), (reg)) | (bits))
#define HCLR2(sc, reg, bits)						\
	HWRITE2((sc), (reg), HREAD2((sc), (reg)) & ~(bits))


struct cfattach exiic_ca = {
	sizeof(struct exiic_softc), NULL, exiic_attach, exiic_detach
};

struct cfdriver exiic_cd = {
	NULL, "exiic", DV_DULL
};

void
exiic_attach(struct device *parent, struct device *self, void *args)
{
	struct exiic_softc *sc = (struct exiic_softc *)self;
	struct ex_attach_args *ea = args;

	sc->sc_iot = ea->ea_iot;
	sc->sc_ios = ea->ea_dev->mem[0].size;
	sc->unit = ea->ea_dev->unit;
	if (bus_space_map(sc->sc_iot, ea->ea_dev->mem[0].addr,
	    ea->ea_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("exiic_attach: bus_space_map failed!");

#if 0
	sc->sc_ih = arm_intr_establish(ea->ea_dev->irq[0], IPL_BIO,
	    exiic_intr, sc, sc->sc_dev.dv_xname);
#endif

	printf("\n");

	/* XXX: set gpio pins */

#if 0
	/* set speed to 100kHz */
	exiic_setspeed(sc, 100);

	/* reset */
	HWRITE2(sc, I2C_I2CR, 0);
	HWRITE2(sc, I2C_I2SR, 0);
#endif

	sc->stopped = 1;
	rw_init(&sc->sc_buslock, sc->sc_dev.dv_xname);

	struct i2cbus_attach_args iba;

	sc->i2c_tag.ic_cookie = sc;
	sc->i2c_tag.ic_acquire_bus = exiic_i2c_acquire_bus;
	sc->i2c_tag.ic_release_bus = exiic_i2c_release_bus;
	sc->i2c_tag.ic_exec = exiic_i2c_exec;

	bzero(&iba, sizeof iba);
	iba.iba_name = "iic";
	iba.iba_tag = &sc->i2c_tag;
	config_found(&sc->sc_dev, &iba, NULL);
}

#if 0
void
exiic_setspeed(struct exiic_softc *sc, u_int speed)
{
	if (!sc->frequency) {
		uint32_t i2c_clk_rate;
		uint32_t div;
		int i;

		i2c_clk_rate = 0; /* XXX */
		div = (i2c_clk_rate + speed - 1) / speed;
		if (div < exiic_clk_div[0][0])
			i = 0;
		else if (div > exiic_clk_div[49][0])
			i = 49;
		else
			for (i = 0; exiic_clk_div[i][0] < div; i++);

		sc->frequency = exiic_clk_div[i][1];
	}

	HWRITE2(sc, I2C_IFDR, sc->frequency);
}
#endif

#if 0
int
exiic_intr(void *arg)
{
	struct exiic_softc *sc = arg;
	u_int16_t status;

	status = HREAD2(sc, I2C_I2SR);

	if (ISSET(status, I2C_I2SR_IIF)) {
		/* acknowledge the interrupts */
		HWRITE2(sc, I2C_I2SR,
		    HREAD2(sc, I2C_I2SR) & ~I2C_I2SR_IIF);

		sc->intr_status |= status;
		wakeup(&sc->intr_status);
	}

	return (0);
}

int
exiic_wait_intr(struct exiic_softc *sc, int mask, int timo)
{
	int status;
	int s;

	s = splbio();

	status = sc->intr_status & mask;
	while (status == 0) {
		if (tsleep(&sc->intr_status, PWAIT, "hcintr", timo)
		    == EWOULDBLOCK) {
			break;
		}
		status = sc->intr_status & mask;
	}
	status = sc->intr_status & mask;
	sc->intr_status &= ~status;

	splx(s);
	return status;
}
#endif

#if 0
int
exiic_wait_state(struct exiic_softc *sc, uint32_t mask, uint32_t value)
{
	uint32_t state;
	int timeout;
	state = HREAD2(sc, I2C_I2SR);
	for (timeout = 1000; timeout > 0; timeout--) {
		if (((state = HREAD2(sc, I2C_I2SR)) & mask) == value)
			return 0;
		delay(10);
	}
	return ETIMEDOUT;
}

int
exiic_read(struct exiic_softc *sc, int addr, int subaddr, void *data, int len)
{
	int i;

	HWRITE2(sc, I2C_I2DR, addr | 1);

	if (exiic_wait_state(sc, I2C_I2SR_IIF, I2C_I2SR_IIF))
		return (EIO);
	while(!(HREAD2(sc, I2C_I2SR) & I2C_I2SR_IIF));
	if (HREAD2(sc, I2C_I2SR) & I2C_I2SR_RXAK)
		return (EIO);

	HCLR2(sc, I2C_I2CR, I2C_I2CR_MTX);
	if (len)
		HCLR2(sc, I2C_I2CR, I2C_I2CR_TXAK);

	/* dummy read */
	HREAD2(sc, I2C_I2DR);

	for (i = 0; i < len; i++) {
		if (exiic_wait_state(sc, I2C_I2SR_IIF, I2C_I2SR_IIF))
			return (EIO);
		if (i == (len - 1)) {
			HCLR2(sc, I2C_I2CR, I2C_I2CR_MSTA | I2C_I2CR_MTX);
			exiic_wait_state(sc, I2C_I2SR_IBB, 0);
			sc->stopped = 1;
		} else if (i == (len - 2)) {
			HSET2(sc, I2C_I2CR, I2C_I2CR_TXAK);
		}
		((uint8_t*)data)[i] = HREAD2(sc, I2C_I2DR);
	}

	return 0;
}

int
exiic_write(struct exiic_softc *sc, int addr, int subaddr, const void *data, int len)
{
	int i;

	HWRITE2(sc, I2C_I2DR, addr);

	if (exiic_wait_state(sc, I2C_I2SR_IIF, I2C_I2SR_IIF))
		return (EIO);
	if (HREAD2(sc, I2C_I2SR) & I2C_I2SR_RXAK)
		return (EIO);

	for (i = 0; i < len; i++) {
		HWRITE2(sc, I2C_I2DR, ((uint8_t*)data)[i]);
		if (exiic_wait_state(sc, I2C_I2SR_IIF, I2C_I2SR_IIF))
			return (EIO);
		if (HREAD2(sc, I2C_I2SR) & I2C_I2SR_RXAK)
			return (EIO);
	}
	return 0;
}
#endif

int
exiic_i2c_acquire_bus(void *cookie, int flags)
{
	struct exiic_softc *sc = cookie;

	return (rw_enter(&sc->sc_buslock, RW_WRITE));
}

void
exiic_i2c_release_bus(void *cookie, int flags)
{
	struct exiic_softc *sc = cookie;

	(void) rw_exit(&sc->sc_buslock);
}

int
exiic_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
#if 0
	struct exiic_softc *sc = cookie;
	uint32_t ret = 0;
	u_int8_t cmd = 0;

	if (!I2C_OP_STOP_P(op) || cmdlen > 1)
		return (EINVAL);

	if (cmdlen > 0)
		cmd = *(u_int8_t *)cmdbuf;

	addr &= 0x7f;

	/* clock gating */
	//exccm_enable_i2c(sc->unit);

	/* set speed to 100kHz */
	exiic_setspeed(sc, 100);

	/* enable the controller */
	HWRITE2(sc, I2C_I2SR, 0);
	HWRITE2(sc, I2C_I2CR, I2C_I2CR_IEN);

	/* wait for it to be stable */
	delay(50);

	/* start transaction */
	HSET2(sc, I2C_I2CR, I2C_I2CR_MSTA);

	if (exiic_wait_state(sc, I2C_I2SR_IBB, I2C_I2SR_IBB)) {
		ret = (EIO);
		goto fail;
	}

	sc->stopped = 0;

	HSET2(sc, I2C_I2CR, I2C_I2CR_IIEN | I2C_I2CR_MTX | I2C_I2CR_TXAK);

	if (I2C_OP_READ_P(op)) {
		if (exiic_read(sc, (addr << 1), cmd, buf, len) != 0)
			ret = (EIO);
	} else {
		if (exiic_write(sc, (addr << 1), cmd, buf, len) != 0)
			ret = (EIO);
	}

fail:
	if (!sc->stopped) {
		HCLR2(sc, I2C_I2CR, I2C_I2CR_MSTA | I2C_I2CR_MTX);
		exiic_wait_state(sc, I2C_I2SR_IBB, 0);
		sc->stopped = 1;
	}

	HWRITE2(sc, I2C_I2CR, 0);

	return ret;
#endif
	return 0;
}

int
exiic_detach(struct device *self, int flags)
{
	struct exiic_softc *sc = (struct exiic_softc *)self;

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return 0;
}
