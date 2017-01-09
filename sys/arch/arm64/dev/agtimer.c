/* $OpenBSD: agtimer.c,v 1.3 2015/05/29 05:48:07 jsg Exp $ */
/*
 * Copyright (c) 2011,2015 Dale Rahn <drahn@dalerahn.com>
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

/* registers */
#define GTIMER_CNTP_CTL_ENABLE		(1 << 0)
#define GTIMER_CNTP_CTL_IMASK		(1 << 1)
#define GTIMER_CNTP_CTL_ISTATUS		(1 << 2)

#define GTIMER_CNTKCTL_PL0PCTEN		(1 << 0) /* PL0 CNTPCT and CNTFRQ access */
#define GTIMER_CNTKCTL_PL0VCTEN		(1 << 1) /* PL0 CNTVCT and CNTFRQ access */
#define GTIMER_CNTKCTL_EVNTEN		(1 << 2) /* Enables virtual counter events */
#define GTIMER_CNTKCTL_EVNTDIR		(1 << 3) /* Virtual counter event transition */
#define GTIMER_CNTKCTL_EVNTI		(0xf << 4) /* Virtual counter event bits */
#define GTIMER_CNTKCTL_PL0VTEN		(1 << 8) /* PL0 Virtual timer reg access */
#define GTIMER_CNTKCTL_PL0PTEN		(1 << 9) /* PL0 Physical timer reg access */

int32_t agtimer_frequency = 0;

u_int agtimer_get_timecount(struct timecounter *);

static struct timecounter agtimer_timecounter = {
	agtimer_get_timecount, NULL, 0x7fffffff, 0, "agtimer", 0, NULL
};

#define MAX_ARM_CPUS	8

struct agtimer_pcpu_softc {
	uint64_t 		pc_nexttickevent;
	uint64_t 		pc_nextstatevent;
	u_int32_t		pc_ticks_err_sum;
};

struct agtimer_softc {
	struct device		sc_dev;
	uint32_t		sc_physical;

	struct agtimer_pcpu_softc sc_pstat[MAX_ARM_CPUS];

	u_int32_t		sc_ticks_err_cnt;
	u_int32_t		sc_ticks_per_second;
	u_int32_t		sc_ticks_per_intr;
	u_int32_t		sc_statvar;
	u_int32_t		sc_statmin;

#ifdef AMPTIMER_DEBUG
	struct evcount		sc_clk_count;
	struct evcount		sc_stat_count;
#endif
};

int		agtimer_match(struct device *, void *, void *);
int		agtimer_fdt_match(struct device *, void *, void *);
void		agtimer_attach(struct device *, struct device *, void *);
uint64_t	agtimer_readcnt64(struct agtimer_softc *sc);
int		agtimer_intr(void *);
void		agtimer_cpu_initclocks(void);
void		agtimer_delay(u_int);
void		agtimer_setstatclockrate(int stathz);
void		agtimer_set_clockrate(int32_t new_frequency);
void		agtimer_startclock(void);

/* hack - XXXX
 * agtimer connects directly to ampintc, not thru the generic
 * inteface because it uses an 'internal' interupt
 * not a peripheral interrupt.
 */
void	*ampintc_intr_establish(int, int, int (*)(void *), void *, char *);


// XXX
void
isb(void)
{
	__asm("isb");
}


struct cfattach agtimer_ca = {
	sizeof (struct agtimer_softc), agtimer_match, agtimer_attach
};

struct cfattach agtimer_fdt_ca = {
	sizeof (struct agtimer_softc), agtimer_fdt_match, agtimer_attach
};

struct cfdriver agtimer_cd = {
	NULL, "agtimer", DV_DULL
};

static int
agtimer_get_freq()
{
	uint32_t val;

	__asm volatile("MRS %x0, CNTFRQ_EL0" : "=r" (val));

	return (val);
}

uint64_t
agtimer_readcnt64(struct agtimer_softc *sc)
{
	uint64_t val;

	isb();
	if (sc->sc_physical)
		__asm volatile("MRS %x0, CNTPCT_EL0" : "=r" (val));
	else 
		__asm volatile("MRS %x0, CNTVCT_EL0" : "=r" (val));

	return (val);
}

static inline int
agtimer_get_ctrl(struct agtimer_softc *sc)
{
	uint32_t val;

	if (sc->sc_physical)
		__asm volatile("MRS %x0, CNTP_CTL_EL0" : "=r" (val));
	else 
		__asm volatile("MRS %x0, CNTV_CTL_EL0" : "=r" (val));

	return (val);
}

static inline int
agtimer_set_ctrl(struct agtimer_softc *sc, uint32_t val)
{
	if (sc->sc_physical)
		__asm volatile("MSR CNTP_CTL_EL0, %x0" :: "r" (val));
	else
		__asm volatile("MSR CNTV_CTL_EL0, %x0" :: "r" (val));
	isb();

	return (0);
}

static inline int
agtimer_set_tval(struct agtimer_softc *sc, uint32_t val)
{
	if (sc->sc_physical)
		__asm volatile("MSR CNTP_TVAL_EL0, %x0" : : [val] "r" (val));
	else
		__asm volatile("MSR CNTV_TVAL_EL0, %x0" : : [val] "r" (val));

	isb();

	return (0);
}

static inline void
agtimer_disable_user_access(void)
{
	uint32_t cntkctl;

	#if 0
	__asm volatile("mrc p15, 0, %0, c14, c1, 0" : "=r" (cntkctl));
	cntkctl &= ~(GTIMER_CNTKCTL_PL0PCTEN | GTIMER_CNTKCTL_PL0VCTEN |
	    GTIMER_CNTKCTL_EVNTEN | GTIMER_CNTKCTL_PL0VTEN |
	    GTIMER_CNTKCTL_PL0PTEN);
	__asm volatile("mcr p15, 0, %0, c14, c1, 0" : : "r" (cntkctl));
	#endif
	__asm volatile("MRS %x0, CNTKCTL_EL1" : "=r" (cntkctl));
	cntkctl &= ~(GTIMER_CNTKCTL_PL0PCTEN | GTIMER_CNTKCTL_PL0VCTEN |
	    GTIMER_CNTKCTL_EVNTEN | GTIMER_CNTKCTL_PL0VTEN |
	    GTIMER_CNTKCTL_PL0PTEN);
	__asm volatile("MSR CNTKCTL_EL1, %x0" : : "r" (cntkctl));

	isb();
}

int
agtimer_match(struct device *parent, void *cfdata, void *aux)
{
#if 0
	if ((cpufunc_id() & CPU_ID_CORTEX_A7_MASK) == CPU_ID_CORTEX_A7 ||
	    (cpufunc_id() & CPU_ID_CORTEX_A15_MASK) == CPU_ID_CORTEX_A15 ||
	    (cpufunc_id() & CPU_ID_CORTEX_A17_MASK) == CPU_ID_CORTEX_A17)
		return (1);
#endif

	return 0;
}

int
agtimer_fdt_match(struct device *parent, void *cfdata, void *aux)
{
	struct arm64_attach_args *ca = aux;

	// XXX  arm64
	if (fdt_node_compatible("arm,armv7-timer", ca->aa_node))
		return (1);
	if (fdt_node_compatible("arm,armv8-timer", ca->aa_node))
		return (1);

	return 0;
}

void
agtimer_attach(struct device *parent, struct device *self, void *args)
{
	struct agtimer_softc *sc = (struct agtimer_softc *)self;
	struct arm64_attach_args *ia = args;
	void *node = ia->aa_node;

	if (node)
		fdt_node_property_int(node, "clock-frequency",
		    &agtimer_frequency);

	if (agtimer_frequency)
		sc->sc_ticks_per_second = agtimer_frequency;

	if (!sc->sc_ticks_per_second)
		sc->sc_ticks_per_second = agtimer_get_freq();

	if (!sc->sc_ticks_per_second)
		panic("%s: no clock frequency specified", self->dv_xname);

	printf(": tick rate %d KHz\n", sc->sc_ticks_per_second /1000);

	/*
	 * The virtual timers are always available.
	 *
	 * When available, the physical timer is a better solution.
	 * For that we need to know that we have been booted in the
	 * hypervisor mode. So far we should be fine using the virtual
	 * timer only. Revisit this for e.g. RK3288.
	 */
	sc->sc_physical = 1; // XXX 

	/* Disable access from PL0/userland. */
	agtimer_disable_user_access();

#ifdef AMPTIMER_DEBUG
	evcount_attach(&sc->sc_clk_count, "clock", NULL);
	evcount_attach(&sc->sc_stat_count, "stat", NULL);
#endif

	/* establish interrupts */
	/* TODO: Add interrupt FDT API. */
	if (node) {
		/* Setup secure, non-secure and virtual timer IRQs. */
		for (int i = 0; i < 4; i++) {
			arm_intr_establish_fdt_idx(node, i, IPL_CLOCK,
			    agtimer_intr, NULL, "tick");
		}
	} else {
#if 0
		ampintc_intr_establish(27, IPL_CLOCK, agtimer_intr,
		    NULL, "tick");
		ampintc_intr_establish(29, IPL_CLOCK, agtimer_intr,
		    NULL, "tick");
		ampintc_intr_establish(30, IPL_CLOCK, agtimer_intr,
		    NULL, "tick");
#else
		panic("no interrupt establish for non-fdt");
#endif
	}

	/*
	 * private timer and interrupts not enabled until
	 * timer configures
	 */

	arm_clock_register(agtimer_cpu_initclocks, agtimer_delay,
	    agtimer_setstatclockrate, agtimer_startclock);

	agtimer_timecounter.tc_frequency = sc->sc_ticks_per_second;
	agtimer_timecounter.tc_priv = sc;

	tc_init(&agtimer_timecounter);
}

u_int
agtimer_get_timecount(struct timecounter *tc)
{
	struct agtimer_softc *sc = agtimer_timecounter.tc_priv;
	return agtimer_readcnt64(sc);
}

int
agtimer_intr(void *frame)
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];
	struct agtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
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

	ctrl = agtimer_get_ctrl(sc);
	if (!(ctrl & GTIMER_CNTP_CTL_ISTATUS))
		return (rc);

	/*
	 * DSR - I know that the tick timer is 64 bits, but the following
	 * code deals with rollover, so there is no point in dealing
	 * with the 64 bit math, just let the 32 bit rollover
	 * do the right thing
	 */

	now = agtimer_readcnt64(sc);

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

	agtimer_set_tval(sc, delay);

	return (rc);
}

void
agtimer_set_clockrate(int32_t new_frequency)
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];

	if (sc == NULL)
		return;

	sc->sc_ticks_per_second = new_frequency;
	agtimer_timecounter.tc_frequency = sc->sc_ticks_per_second;
	printf("%s: adjusting clock: new tick rate %d KHz\n",
	    sc->sc_dev.dv_xname, sc->sc_ticks_per_second /1000);
}

void
agtimer_cpu_initclocks()
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];
	struct agtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint32_t		 reg;
	uint64_t		 next;

	stathz = hz;
	profhz = hz * 10;

	if (agtimer_frequency)
		agtimer_set_clockrate(agtimer_frequency);

	agtimer_setstatclockrate(stathz);

	sc->sc_ticks_per_intr = sc->sc_ticks_per_second / hz;
	sc->sc_ticks_err_cnt = sc->sc_ticks_per_second % hz;
	pc->pc_ticks_err_sum = 0;

	next = agtimer_readcnt64(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = next;

	reg = agtimer_get_ctrl(sc);
	reg &= GTIMER_CNTP_CTL_IMASK;
	reg |= GTIMER_CNTP_CTL_ENABLE;
	agtimer_set_tval(sc, sc->sc_ticks_per_intr);
	agtimer_set_ctrl(sc, reg);
}

void
agtimer_delay(u_int usecs)
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];
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

	oclock = agtimer_readcnt64(sc);
	while (1) {
		for (j = 100; j > 0; j--)
			;
		clock = agtimer_readcnt64(sc);
		delta = clock - oclock;
		if (delta > delaycnt)
			break;
	}
}

void
agtimer_setstatclockrate(int newhz)
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];
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
agtimer_startclock(void)
{
	struct agtimer_softc	*sc = agtimer_cd.cd_devs[0];
	struct agtimer_pcpu_softc *pc = &sc->sc_pstat[CPU_INFO_UNIT(curcpu())];
	uint64_t nextevent;
	uint32_t reg;

	nextevent = agtimer_readcnt64(sc) + sc->sc_ticks_per_intr;
	pc->pc_nexttickevent = pc->pc_nextstatevent = nextevent;

	reg = agtimer_get_ctrl(sc);
	reg &= GTIMER_CNTP_CTL_IMASK;
	reg |= GTIMER_CNTP_CTL_ENABLE;
	agtimer_set_tval(sc, sc->sc_ticks_per_intr);
	agtimer_set_ctrl(sc, reg);
}
