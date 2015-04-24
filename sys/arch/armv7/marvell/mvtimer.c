/*
 * Copyright (c) 2011 Dale Rahn <drahn@openbsd.org>
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
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/evcount.h>
#include <sys/stdint.h>

#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

/* registers */
#define TIMER_CTRL		0x0000
#define		TIMER0_ENABLE		(1 << 0)
#define		TIMER0_RELOAD_ENABLE	(1 << 1)
#define		TIMER1_ENABLE		(1 << 2)
#define		TIMER1_RELOAD_ENABLE	(1 << 3)
#define		TIMER0_25MHZ		(1 << 11)
#define		TIMER1_25MHZ		(1 << 12)
#define TIMER_EVENTS_STATUS	0x0004
#define		TIMER0_CLR_MASK			(~0x1)
#define		TIMER1_CLR_MASK			(~0x100)
#define TIMER0_RELOAD		0x0010
#define TIMER0_VALUE		0x0014
#define TIMER1_RELOAD		0x0018
#define TIMER1_VALUE		0x001c

#define LCL_TIMER_EVENTS_STATUS	0x0028

u_int mvtimer_get_timecount(struct timecounter *);

static struct timecounter mvtimer_timecounter = {
	mvtimer_get_timecount, NULL, 0xffffffff, 0, "mvtimer", 0, NULL
};

#define MAX_ARM_CPUS	8

struct mvtimer_pcpu_softc {
	uint32_t 		pc_nexttickevent;
	uint32_t 		pc_nextstatevent;
	u_int32_t		pc_ticks_err_sum;
};

struct mvtimer_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_pioh;
	void *			sc_node;

	struct mvtimer_pcpu_softc sc_pstat[MAX_ARM_CPUS];

	u_int32_t		sc_ticks_err_cnt;
	u_int32_t		sc_ticks_per_second;
	u_int32_t		sc_ticks_per_intr;
	u_int32_t		sc_statvar;
	u_int32_t		sc_statmin;

#ifdef MVTIMER_DEBUG
	struct evcount		sc_clk_count;
	struct evcount		sc_stat_count;
#endif
};

int		mvtimer_match(struct device *, void *, void *);
void		mvtimer_attach(struct device *, struct device *, void *);
uint32_t	mvtimer_readcnt(struct mvtimer_softc *sc);
int		mvtimer_intr(void *);
void		mvtimer_cpu_initclocks(void);
void		mvtimer_delay(u_int);
void		mvtimer_setstatclockrate(int stathz);
void		mvtimer_startclock(void);

struct cfattach mvtimer_ca = {
	sizeof (struct mvtimer_softc), mvtimer_match, mvtimer_attach
};

struct cfdriver mvtimer_cd = {
	NULL, "mvtimer", DV_DULL
};

int
mvtimer_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	/* TODO: support other timer */
	if (fdt_node_compatible("marvell,armada-xp-timer", aa->aa_node))
		return (1);

	return 0;
}

void
mvtimer_attach(struct device *parent, struct device *self, void *args)
{
	struct mvtimer_softc *sc = (struct mvtimer_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	struct clk *clk;

	sc->sc_iot = aa->aa_iot;
	sc->sc_node = aa->aa_node;

	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: cannot extract global memory", sc->sc_dev.dv_xname);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map global timer failed!", __func__);

	if (fdt_get_memory_address(aa->aa_node, 1, &mem))
		panic("%s: cannot extract local memory", sc->sc_dev.dv_xname);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_pioh))
		panic("%s: bus_space_map priv timer failed!", __func__);

	clk = clk_fdt_get_by_name(aa->aa_node, "fixed");
	if (clk == NULL)
		panic("%s: cannot get clock", __func__);

	sc->sc_ticks_per_second = clk_get_rate(clk) * 1000;
	printf(": tick rate %d KHz\n", sc->sc_ticks_per_second / 1000);

	/* disable timers and clear */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIMER_CTRL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_pioh, LCL_TIMER_EVENTS_STATUS,
	    TIMER0_CLR_MASK);

	/* set 25MHz (TODO: depending on SoC) */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, TIMER_CTRL) |
	    TIMER0_25MHZ);
	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL) |
	    TIMER0_25MHZ);

	/* enable global timer */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIMER0_VALUE, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIMER0_RELOAD, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, TIMER_CTRL) |
	    (TIMER0_RELOAD_ENABLE | TIMER0_ENABLE));

#ifdef MVTIMER_DEBUG
	evcount_attach(&sc->sc_clk_count, "clock", NULL);
	evcount_attach(&sc->sc_stat_count, "stat", NULL);
#endif

	/*
	 * private timer and interrupts not enabled until
	 * timer configures
	 */

	arm_clock_register(mvtimer_cpu_initclocks, mvtimer_delay,
	    mvtimer_setstatclockrate, mvtimer_startclock);

	mvtimer_timecounter.tc_frequency = sc->sc_ticks_per_second;
	mvtimer_timecounter.tc_priv = sc;

	tc_init(&mvtimer_timecounter);
}

u_int
mvtimer_get_timecount(struct timecounter *tc)
{
	struct mvtimer_softc *sc = mvtimer_timecounter.tc_priv;
	return UINT32_MAX - bus_space_read_4(sc->sc_iot, sc->sc_ioh, TIMER0_VALUE);
}

uint32_t
mvtimer_readcnt(struct mvtimer_softc *sc)
{
	return UINT32_MAX - bus_space_read_4(sc->sc_iot, sc->sc_ioh, TIMER0_VALUE);
}

int
mvtimer_intr(void *frame)
{
	struct mvtimer_softc	*sc = mvtimer_cd.cd_devs[0];
	struct mvtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint32_t		 now;
	uint32_t		 nextevent;
	uint32_t		 r;
	int32_t			 delay;
	int			 rc = 0;

	/*
	 * DSR - I know that the tick timer is 64 bits, but the following
	 * code deals with rollover, so there is no point in dealing
	 * with the 64 bit math, just let the 32 bit rollover
	 * do the right thing
	 */

	now = mvtimer_readcnt(sc);

	while ((int32_t) (pc->pc_nexttickevent - now) <= 0) {
		pc->pc_nexttickevent += sc->sc_ticks_per_intr;
		pc->pc_ticks_err_sum += sc->sc_ticks_err_cnt;

		/* looping a few times is faster than divide */
		while (pc->pc_ticks_err_sum > hz) {
			pc->pc_nexttickevent += 1;
			pc->pc_ticks_err_sum -= hz;
		}

#ifdef MVTIMER_DEBUG
		sc->sc_clk_count.ec_count++;
#endif
		rc = 1;
		hardclock(frame);
	}
	while ((int32_t) (pc->pc_nextstatevent - now) <= 0) {
		do {
			r = random() & (sc->sc_statvar -1);
		} while (r == 0); /* random == 0 not allowed */
		pc->pc_nextstatevent += sc->sc_statmin + r;

		/* XXX - correct nextstatevent? */
#ifdef MVTIMER_DEBUG
		sc->sc_stat_count.ec_count++;
#endif
		rc = 1;
		statclock(frame);
	}

	if ((pc->pc_nexttickevent - now) < (pc->pc_nextstatevent - now))
		nextevent = pc->pc_nexttickevent;
	else
		nextevent = pc->pc_nextstatevent;

	/* clear old status */
	bus_space_write_4(sc->sc_iot, sc->sc_pioh, LCL_TIMER_EVENTS_STATUS,
	    TIMER0_CLR_MASK);

	delay = nextevent - now;
	if (delay <= 0)
		delay = 1;

	if (delay > sc->sc_ticks_per_intr + 1) {
		printf("%s: time lost!\n", __func__);
		delay = sc->sc_ticks_per_intr;
		pc->pc_nexttickevent = now;
		pc->pc_nextstatevent = now;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER0_VALUE, delay);

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL) | TIMER0_ENABLE);

	return (rc);
}

void
mvtimer_cpu_initclocks()
{
	struct mvtimer_softc	*sc = mvtimer_cd.cd_devs[0];
	struct mvtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint32_t		 next;

	stathz = hz;
	profhz = hz * 10;

	mvtimer_setstatclockrate(stathz);

	sc->sc_ticks_per_intr = sc->sc_ticks_per_second / hz;
	sc->sc_ticks_err_cnt = sc->sc_ticks_per_second % hz;
	pc->pc_ticks_err_sum = 0;

	/* establish interrupts */
	arm_intr_establish_fdt_idx(sc->sc_node, 4, IPL_CLOCK,
	    mvtimer_intr, NULL, "tick");

	next = mvtimer_readcnt(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = next;

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, LCL_TIMER_EVENTS_STATUS,
	    TIMER0_CLR_MASK);

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER0_VALUE,
	    sc->sc_ticks_per_intr);

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL) | TIMER0_ENABLE);
}

void
mvtimer_delay(u_int usecs)
{
	struct mvtimer_softc	*sc = mvtimer_cd.cd_devs[0];
	u_int32_t		clock, oclock, delta, delaycnt;
	volatile int		j;
	int			csec, usec;

	if (usecs > (0x80000000 / (sc->sc_ticks_per_second))) {
		csec = usecs / 10000;
		usec = usecs % 10000;

		delaycnt = (sc->sc_ticks_per_second / 100) * csec +
		    (sc->sc_ticks_per_second / 100) * usec / 10000;
	} else {
		delaycnt = sc->sc_ticks_per_second * usecs / 1000000;
	}
	if (delaycnt <= 1)
		for (j = 100; j > 0; j--)
			;

	oclock = mvtimer_readcnt(sc);
	while (1) {
		for (j = 100; j > 0; j--)
			;
		clock = mvtimer_readcnt(sc);
		delta = clock - oclock;
		if (delta > delaycnt)
			break;
	}
}

void
mvtimer_setstatclockrate(int newhz)
{
	struct mvtimer_softc	*sc = mvtimer_cd.cd_devs[0];
	int			 minint, statint;
	int			 s;

	s = splclock();

	statint = sc->sc_ticks_per_second / newhz;
	/* calculate largest 2^n which is smaller that just over half statint */
	sc->sc_statvar = 0x40000000; /* really big power of two */
	minint = statint / 2 + 100;
	while (sc->sc_statvar > minint)
		sc->sc_statvar >>= 1;

	sc->sc_statmin = statint - (sc->sc_statvar >> 1);

	splx(s);

	/*
	 * XXX this allows the next stat timer to occur then it switches
	 * to the new frequency. Rather than switching instantly.
	 */
}

void
mvtimer_startclock(void)
{
	struct mvtimer_softc	*sc = mvtimer_cd.cd_devs[0];
	struct mvtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint32_t nextevent;

	nextevent = mvtimer_readcnt(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = nextevent;

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, LCL_TIMER_EVENTS_STATUS,
	    TIMER0_CLR_MASK);

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER0_VALUE,
		sc->sc_ticks_per_intr);

	bus_space_write_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL,
	    bus_space_read_4(sc->sc_iot, sc->sc_pioh, TIMER_CTRL) | TIMER0_ENABLE);
}
