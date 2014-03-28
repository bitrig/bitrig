/*	$OpenBSD: sxitimer.c,v 1.2 2013/10/27 12:58:53 jasper Exp $	*/
/*
 * Copyright (c) 2007,2009 Dale Rahn <drahn@openbsd.org>
 * Copyright (c) 2013 Raphael Graf <r@undefined.ch>
 * Copyright (c) 2013,2014 Artturi Alm
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/evcount.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <dev/clock_subr.h>

#include <arm/cpufunc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/sunxi/sunxireg.h>

#define	TIMER_IER 		0x00
#define	TIMER_ISR 		0x04
#define	TIMER_IRQ(x)		(1 << (x))

#define	TIMER_CTRL(x)		(0x10 + (0x10 * (x)))
#define	TIMER_INTV(x)		(0x14 + (0x10 * (x)))
#define	TIMER_CURR(x)		(0x18 + (0x10 * (x)))

/* A20 counter, relative to CPUCNTRS_ADDR */
#define	OSC24M_CNT64_CTRL	0x80
#define	OSC24M_CNT64_LOW	0x84
#define	OSC24M_CNT64_HIGH	0x88

/* A1X counter */
#define	CNT64_CTRL		0xa0
#define	CNT64_LOW		0xa4
#define	CNT64_HIGH		0xa8

#define	CNT64_CLR_EN		(1 << 0) /* clear enable */
#define	CNT64_RL_EN		(1 << 1) /* read latch enable */
#define	CNT64_SYNCH		(1 << 4) /* sync to OSC24M counter */

#define	LOSC_CTRL		0x100
#define	OSC32K_SRC_SEL		(1 << 0)

#define	TIMER_ENABLE		(1 << 0)
#define	TIMER_RELOAD		(1 << 1)
#define	TIMER_CLK_SRC_MASK	(3 << 2)
#define	TIMER_LSOSC		(0 << 2)
#define	TIMER_OSC24M		(1 << 2)
#define	TIMER_PLL6_6		(2 << 2)
#define	TIMER_PRESC_1		(0 << 4)
#define	TIMER_PRESC_2		(1 << 4)
#define	TIMER_PRESC_4		(2 << 4)
#define	TIMER_PRESC_8		(3 << 4)
#define	TIMER_PRESC_16		(4 << 4)
#define	TIMER_PRESC_32		(5 << 4)
#define	TIMER_PRESC_64		(6 << 4)
#define	TIMER_PRESC_128		(7 << 4)
#define	TIMER_CONTINOUS		(0 << 7)
#define	TIMER_SINGLESHOT	(1 << 7)

#define	TIMER_MIN_CYCLES	0x20	/* 'feel good' value */

void	sxitimer_attach(struct device *, struct device *, void *);
int	sxitimer_intr(void *);
void	sxitimer_cpu_initclocks(void);
void	sxitimer_setstatclockrate(int);
uint64_t	sxitimer_readcnt64(void);
void	sxitimer_delay(u_int);

u_int sxitimer_get_timecount(struct timecounter *);

static struct timecounter sxitimer_timecounter = {
	sxitimer_get_timecount, NULL, 0xffffffff, COUNTER_FREQUENCY,
	"sxitimer", 0, NULL
};

bus_space_tag_t		sxitimer_iot;
bus_space_handle_t	sxitimer_ioh;
bus_space_handle_t	sxitimer_cntr_ioh;

uint32_t tick_tpi;
uint64_t tick_nextevt;

bus_addr_t cntr64_ctrl = CNT64_CTRL;
bus_addr_t cntr64_low = CNT64_LOW;
bus_addr_t cntr64_high = CNT64_HIGH;

struct cfattach	sxitimer_ca = {
	sizeof(struct device), NULL, sxitimer_attach
};

struct cfdriver sxitimer_cd = {
	NULL, "sxitimer", DV_DULL
};

void
sxitimer_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;

	if (self->dv_unit != 0)
		return;

	sxitimer_iot = aa->aa_iot;

	if (bus_space_map(sxitimer_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sxitimer_ioh))
		panic("sxitimer_attach: timer bus_space_map failed");

	if (board_id == BOARD_ID_SUN7I_A20) {
		uint32_t v;

		if (bus_space_map(sxitimer_iot, CPUCNTRS_ADDR, CPUCNTRS_SIZE,
		    0, &sxitimer_cntr_ioh))
			panic("sxitimer_attach: counter bus_space_map failed");

		cntr64_ctrl = OSC24M_CNT64_CTRL;
		cntr64_low = OSC24M_CNT64_LOW;
		cntr64_high = OSC24M_CNT64_HIGH;

		v = bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh,
		    cntr64_ctrl);
		bus_space_write_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_ctrl,
		    v | CNT64_SYNCH);
		bus_space_write_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_ctrl,
		    v & ~CNT64_SYNCH);
	} else
		sxitimer_cntr_ioh = sxitimer_ioh;

	/* clear counter, loop until ready */
	bus_space_write_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_ctrl,
	    CNT64_CLR_EN); /* as a side-effect counter clk_src = OSC24M */
	while (bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_ctrl)
	    & CNT64_CLR_EN)
		continue;

	/* stop timer */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh, TIMER_CTRL(0), 0);

	stathz = profhz = hz;

	tc_init(&sxitimer_timecounter);
	arm_clock_register(sxitimer_cpu_initclocks, sxitimer_delay,
	    sxitimer_setstatclockrate, NULL);

	printf(": tick %dhz clock %dKHz\n", hz, TIMER0_FREQ / 1000);
}

void
sxitimer_cpu_initclocks(void)
{
	uint32_t isr, ier;

	arm_intr_establish(TIMER0_IRQ, IPL_CLOCK, sxitimer_intr, NULL, "tick");

	/* clear pending */
	isr = bus_space_read_4(sxitimer_iot, sxitimer_ioh, TIMER_ISR);
	isr |= TIMER_IRQ(0);
	bus_space_write_4(sxitimer_iot, sxitimer_ioh, TIMER_ISR, isr);

	/* enable interrupt */
	ier = bus_space_read_4(sxitimer_iot, sxitimer_ioh, TIMER_IER);
	ier |= TIMER_IRQ(0);
	bus_space_write_4(sxitimer_iot, sxitimer_ioh, TIMER_IER, ier);

	/* ready */
	tick_tpi = TIMER0_FREQ / hz;
	tick_nextevt = sxitimer_readcnt64() + tick_tpi;

	/* set */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh, TIMER_INTV(0), tick_tpi);

	/* go! */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh,
	    TIMER_CTRL(0),
	    TIMER_ENABLE | TIMER_OSC24M | TIMER_RELOAD);
}

int
sxitimer_intr(void *frame)
{
	uint64_t now;
	int32_t next;

	splassert(IPL_CLOCK);

	now = sxitimer_readcnt64();
	while (tick_nextevt < now) {
		tick_nextevt += tick_tpi;
		hardclock(frame);
	}

	now = sxitimer_readcnt64();
	next = tick_nextevt - now;
	if (next < TIMER_MIN_CYCLES)
		next = TIMER_MIN_CYCLES;
	else if (next > tick_tpi) {
		next = tick_tpi;
		tick_nextevt = now + TIMER_MIN_CYCLES;
	}

	/* clear pending */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh,
	    TIMER_ISR, TIMER_IRQ(0));

	/* set next */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh, TIMER_INTV(0), next);

	/* restart */
	bus_space_write_4(sxitimer_iot, sxitimer_ioh,
	    TIMER_CTRL(0),
	    TIMER_ENABLE | TIMER_OSC24M | TIMER_RELOAD);

	return 1;
}

uint64_t
sxitimer_readcnt64(void)
{
	uint32_t low, high;

	/* latch counter, loop until ready */
	bus_space_write_4(sxitimer_iot, sxitimer_cntr_ioh,
	    cntr64_ctrl, CNT64_RL_EN);
	while (bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_ctrl)
	    & CNT64_RL_EN)
		continue;

	low = bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_low);
	high = bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_high);
	return (uint64_t)high << 32 | low;
}

void
sxitimer_delay(u_int usecs)
{
	uint64_t oclock, timeout;

	oclock = sxitimer_readcnt64();
	timeout = oclock + (COUNTER_FREQUENCY / 1000000) * usecs;

	while (oclock < timeout)
		oclock = sxitimer_readcnt64();
}

void
sxitimer_setstatclockrate(int newhz)
{
}

u_int
sxitimer_get_timecount(struct timecounter *tc)
{
	return bus_space_read_4(sxitimer_iot, sxitimer_cntr_ioh, cntr64_low);
}
