/* $OpenBSD: agmemtimer.c,v 1.3 2015/05/29 05:48:07 jsg Exp $ */
/*
 * Copyright (c) 2011,2015,2017 Dale Rahn <drahn@dalerahn.com>
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
#include <machine/intr.h>
#include <machine/fdt.h>
#include <arm64/arm64/arm64var.h>

#define CNTTIDR		0x08
#define CNTTIDR_VIRT(n)	(1 << ((n) *4))

#define CNTPCT_LO	0x00
#define CNTPCT_HI	0x04
#define CNTVCT_LO	0x08
#define CNTVCT_HI	0x0c
#define CNTFRQ		0x10
#define CNTP_TVAL	0x28
#define CNTP_CTL	0x2c
#define CNTV_TVAL	0x38
#define CNTV_CTL	0x3c

/* registers */
#define GTIMER_CNTP_CTL_ENABLE		(1 << 0)
#define GTIMER_CNTP_CTL_IMASK		(1 << 1)
#define GTIMER_CNTP_CTL_ISTATUS		(1 << 2)


int32_t agmemtimer_frequency = 0;

u_int agmemtimer_get_timecount(struct timecounter *);

static struct timecounter agmemtimer_timecounter = {
	agmemtimer_get_timecount, NULL, 0x7fffffff, 0, "agmemtimer", 0, NULL
};

#define MAX_ARM_CPUS	8

struct agmemtimer_pcpu_softc {
	uint64_t 		pc_nexttickevent;
	uint64_t 		pc_nextstatevent;
	u_int32_t		pc_ticks_err_sum;
	bus_space_handle_t	pc_ioh;
	void 			*pc_frame;
};

struct agmemtimer_softc {
	struct device		sc_dev;
	uint32_t		sc_physical;

	struct agmemtimer_pcpu_softc sc_pstat[MAX_ARM_CPUS];

	u_int32_t		sc_ticks_err_cnt;
	u_int32_t		sc_ticks_per_second;
	u_int32_t		sc_ticks_per_intr;
	u_int32_t		sc_statvar;
	u_int32_t		sc_statmin;

#define AMPTIMER_DEBUG
#ifdef AMPTIMER_DEBUG
	struct evcount		sc_clk_count;
	struct evcount		sc_stat_count;
#endif
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

int		agmemtimer_fdt_match(struct device *, void *, void *);
void		agmemtimer_attach(struct device *, struct device *, void *);
uint64_t	agmemtimer_readcnt64(struct agmemtimer_softc *sc);
int		agmemtimer_intr(void *);
void		agmemtimer_cpu_initclocks(void);
void		agmemtimer_delay(u_int);
void		agmemtimer_setstatclockrate(int stathz);
void		agmemtimer_set_clockrate(int32_t new_frequency);
void		agmemtimer_startclock(void);

struct cfattach agmemtimer_fdt_ca = {
	sizeof (struct agmemtimer_softc), agmemtimer_fdt_match, agmemtimer_attach
};

struct cfdriver agmemtimer_cd = {
	NULL, "agmemtimer", DV_DULL
};



static int
agmemtimer_get_freq(struct agmemtimer_softc *sc)
{
	uint32_t val;

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_pstat[cpu_number()].pc_ioh;

	val = bus_space_read_4(iot, ioh, CNTFRQ);

	return (val);
}

uint64_t
agmemtimer_readcnt64(struct agmemtimer_softc *sc)
{
	uint32_t high0, high1, low;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_pstat[cpu_number()].pc_ioh;
	int highreg, lowreg;

	if (sc->sc_physical) {
		highreg = CNTPCT_HI;
		lowreg = CNTPCT_LO;

	} else {
		highreg = CNTVCT_HI;
		lowreg = CNTVCT_LO;
	}
	
	do {
		high0 = bus_space_read_4(iot, ioh, highreg);
		low = bus_space_read_4(iot, ioh, lowreg);
		high1 = bus_space_read_4(iot, ioh, highreg);
	} while (high0 != high1);

	return ((((uint64_t)high1) << 32) | low);
}

static inline int
agmemtimer_get_ctrl(struct agmemtimer_softc *sc)
{
	uint32_t val;

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_pstat[cpu_number()].pc_ioh;
	int reg;

	if (sc->sc_physical) {
		reg = CNTP_CTL;
	} else {
		reg = CNTV_CTL;
	}

	val = bus_space_read_4(iot, ioh, reg);

	return (val);
}

static inline int
agmemtimer_set_ctrl(struct agmemtimer_softc *sc, uint32_t val)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_pstat[cpu_number()].pc_ioh;
	int reg;

	if (sc->sc_physical) {
		reg = CNTP_CTL;
	} else {
		reg = CNTV_CTL;
	}

	bus_space_write_4(iot, ioh, reg, val);

	return (0);
}

static inline int
agmemtimer_set_tval(struct agmemtimer_softc *sc, uint32_t val)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_pstat[cpu_number()].pc_ioh;
	int reg;

	if (sc->sc_physical) {
		reg = CNTP_TVAL;
	} else {
		reg = CNTV_TVAL;
	}

	bus_space_write_4(iot, ioh, reg, val);

	return (0);
}

int
agmemtimer_fdt_match(struct device *parent, void *cfdata, void *aux)
{
	struct arm64_attach_args *ca = aux;

	if (fdt_node_compatible("arm,armv7-timer-mem", ca->aa_node))
		return (1);

	return 0;
}

void
agmemtimer_attach(struct device *parent, struct device *self, void *args)
{
	struct agmemtimer_softc *sc = (struct agmemtimer_softc *)self;
	struct arm64_attach_args *ia = args;
	void *node = ia->aa_node;
	struct fdt_memory mem;

	if (node)
		fdt_node_property_int(node, "clock-frequency",
		    &agmemtimer_frequency);

	if (agmemtimer_frequency)
		sc->sc_ticks_per_second = agmemtimer_frequency;
	agmemtimer_frequency = 0; // no need for later adjustment

	if (!sc->sc_ticks_per_second)
		sc->sc_ticks_per_second = agmemtimer_get_freq(sc);

	if (!sc->sc_ticks_per_second)
		panic("%s: no clock frequency specified", self->dv_xname);

	printf(": tick rate %d KHz", sc->sc_ticks_per_second /1000);

	sc->sc_physical = 0; // default

	sc->sc_iot = ia->aa_iot;
	if (fdt_get_memory_address(ia->aa_node, 0, &mem)) {
		panic("no memory for timer base");
	}

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("amptimer_attach: bus_space_map timer base failed!");


#ifdef AMPTIMER_DEBUG
	evcount_attach(&sc->sc_clk_count, "clock", NULL);
	evcount_attach(&sc->sc_stat_count, "stat", NULL);
#endif

	// attach a timer (ioh) for each available timer


	int timercnt = 0;
	void *frame;
	fdt_for_each_child_of_node(ia->aa_node, frame) {
		int nodeid;
		if (!fdt_node_property_int(frame, "frame-number", &nodeid))
			continue; // skip if not a frame


		if (sc->sc_pstat[nodeid].pc_frame != NULL) {
//			printf("reattaching frame %d\n", nodeid);
			continue;
		}
		uint32_t val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    CNTTIDR);
		if ((val >> (4 * nodeid)) == 0x3) {
			// not present
			continue;
		}
//		printf("attaching frame %d of agmemtimer\n", nodeid);
		if (val & CNTTIDR_VIRT(nodeid)) {
			// virtual enabled
		} else {
			sc->sc_physical = 1; // virtual not available
		}

		if (fdt_get_memory_address(node, 0, &mem)) {
			panic("no memory for timer frame");
		}

		if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0,
		    &sc->sc_pstat[nodeid].pc_ioh))
			panic("amptimer_attach: bus_space_map timer %d failed!",
			    nodeid);
		timercnt++;
		sc->sc_pstat[nodeid].pc_frame = frame;

	}
	printf(", %d timers\n", timercnt);

	

	/* establish interrupts */
	int timer_id = sc->sc_physical ? 0: 1;
	arm_intr_establish_fdt_idx( sc->sc_pstat[0].pc_frame, timer_id,
	    IPL_CLOCK, agmemtimer_intr, NULL, "tick");

	/*
	 * private timer and interrupts not enabled until
	 * timer configures
	 */

	arm_clock_register(agmemtimer_cpu_initclocks, agmemtimer_delay,
	    agmemtimer_setstatclockrate, agmemtimer_startclock);

	agmemtimer_timecounter.tc_frequency = sc->sc_ticks_per_second;
	agmemtimer_timecounter.tc_priv = sc;

	tc_init(&agmemtimer_timecounter);
}

u_int
agmemtimer_get_timecount(struct timecounter *tc)
{
	struct agmemtimer_softc *sc = agmemtimer_timecounter.tc_priv;
	return agmemtimer_readcnt64(sc);
}

int
agmemtimer_intr(void *frame)
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];
	struct agmemtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint64_t		 now;
	uint64_t		 nextevent;
	uint32_t		 r;
#if defined(USE_GTIMER_CMP)
	int			 skip = 1;
#else
	int64_t			 delay;
#endif
	int			 rc = 0;
	int			 ctrl;

	ctrl = agmemtimer_get_ctrl(sc);
	if (!(ctrl & GTIMER_CNTP_CTL_ISTATUS))
		return (rc);

	/*
	 * DSR - I know that the tick timer is 64 bits, but the following
	 * code deals with rollover, so there is no point in dealing
	 * with the 64 bit math, just let the 32 bit rollover
	 * do the right thing
	 */

	now = agmemtimer_readcnt64(sc);

	while (pc->pc_nexttickevent <= now) {
		pc->pc_nexttickevent += sc->sc_ticks_per_intr;
		pc->pc_ticks_err_sum += sc->sc_ticks_err_cnt;

		/* looping a few times is faster than divide */
		while (pc->pc_ticks_err_sum > hz) {
			pc->pc_nexttickevent += 1;
			pc->pc_ticks_err_sum -= hz;
		}

#ifdef AMPTIMER_DEBUG
		sc->sc_clk_count.ec_count++;
#endif
		rc = 1;
		hardclock(frame);
	}
	while (pc->pc_nextstatevent <= now) {
		do {
			r = random() & (sc->sc_statvar -1);
		} while (r == 0); /* random == 0 not allowed */
		pc->pc_nextstatevent += sc->sc_statmin + r;

		/* XXX - correct nextstatevent? */
#ifdef AMPTIMER_DEBUG
		sc->sc_stat_count.ec_count++;
#endif
		rc = 1;
		statclock(frame);
	}

	if (pc->pc_nexttickevent < pc->pc_nextstatevent)
		nextevent = pc->pc_nexttickevent;
	else
		nextevent = pc->pc_nextstatevent;

	delay = nextevent - now;
	if (delay < 0)
		delay = 1;

	agmemtimer_set_tval(sc, delay);

	return (rc);
}

void
agmemtimer_set_clockrate(int32_t new_frequency)
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];

	if (sc == NULL)
		return;

	sc->sc_ticks_per_second = new_frequency;
	agmemtimer_timecounter.tc_frequency = sc->sc_ticks_per_second;
	printf("%s: adjusting clock: new tick rate %d KHz\n",
	    sc->sc_dev.dv_xname, sc->sc_ticks_per_second /1000);
}

void
agmemtimer_cpu_initclocks()
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];
	struct agmemtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint32_t		 reg;
	uint64_t		 next;

	stathz = hz;
	profhz = hz * 10;

	if (agmemtimer_frequency)
		agmemtimer_set_clockrate(agmemtimer_frequency);

	agmemtimer_setstatclockrate(stathz);

	sc->sc_ticks_per_intr = sc->sc_ticks_per_second / hz;
	sc->sc_ticks_err_cnt = sc->sc_ticks_per_second % hz;
	pc->pc_ticks_err_sum = 0;

	next = agmemtimer_readcnt64(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = next;

	reg = agmemtimer_get_ctrl(sc);
	reg &= GTIMER_CNTP_CTL_IMASK;
	reg |= GTIMER_CNTP_CTL_ENABLE;
	agmemtimer_set_tval(sc, sc->sc_ticks_per_intr);
	agmemtimer_set_ctrl(sc, reg);
}

void
agmemtimer_delay(u_int usecs)
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];
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

	oclock = agmemtimer_readcnt64(sc);
	while (1) {
		for (j = 100; j > 0; j--)
			;
		clock = agmemtimer_readcnt64(sc);
		delta = clock - oclock;
		if (delta > delaycnt)
			break;
	}
}

void
agmemtimer_setstatclockrate(int newhz)
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];
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
agmemtimer_startclock(void)
{
	struct agmemtimer_softc	*sc = agmemtimer_cd.cd_devs[0];
	struct agmemtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint64_t nextevent;
	uint32_t reg;

	nextevent = agmemtimer_readcnt64(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = nextevent;

	reg = agmemtimer_get_ctrl(sc);
	reg &= GTIMER_CNTP_CTL_IMASK;
	reg |= GTIMER_CNTP_CTL_ENABLE;
	agmemtimer_set_tval(sc, sc->sc_ticks_per_intr);
	agmemtimer_set_ctrl(sc, reg);
}
