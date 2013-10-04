/* $OpenBSD: intr.c,v 1.1 2013/09/04 14:38:25 patrick Exp $ */
/*
 * Copyright (c) 2011 Dale Rahn <drahn@openbsd.org>
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
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>
#include <arm/cpufunc.h>
#include <machine/cpu.h>
#include <machine/intr.h>

void *arm_dflt_intr_establish(int irqno, int level, int (*func)(void *),
    void *cookie, char *name);
void arm_dflt_intr_disestablish(void *cookie);
const char *arm_dflt_intr_string(void *cookie);

void arm_dflt_intr(void *);
void arm_intr(void *);


#define SI_TO_IRQBIT(x) (1 << (x))
uint32_t arm_smask[NIPL];

struct arm_intr_func arm_intr_func = {
	arm_dflt_intr_establish,
	arm_dflt_intr_disestablish,
	arm_dflt_intr_string
};

void (*arm_intr_dispatch)(void *) = arm_dflt_intr;

void
arm_intr(void *frame)
{
	/* XXX - change this to have irq_dispatch use function pointer */
	(*arm_intr_dispatch)(frame);
}
void
arm_dflt_intr(void *frame)
{
	panic("arm_dflt_intr() called");
}


void *arm_intr_establish(int irqno, int level, int (*func)(void *),
    void *cookie, char *name)
{
	return arm_intr_func.intr_establish(irqno, level, func, cookie, name);
}
void arm_intr_disestablish(void *cookie)
{
	arm_intr_func.intr_disestablish(cookie);
}
const char *arm_intr_string(void *cookie)
{
	return arm_intr_func.intr_string(cookie);
}

void *arm_dflt_intr_establish(int irqno, int level, int (*func)(void *),
    void *cookie, char *name)
{
	panic("arm_dflt_intr_establish called");
}

void arm_dflt_intr_disestablish(void *cookie)
{
	panic("arm_dflt_intr_disestablish called");
}

const char *
arm_dflt_intr_string(void *cookie)
{
	panic("arm_dflt_intr_string called");
}

void arm_set_intr_handler(
	void *(*intr_establish)(int irqno, int level, int (*func)(void *),
	    void *cookie, char *name),
	void (*intr_disestablish)(void *cookie),
	const char *(intr_string)(void *cookie),
	void (*intr_handle)(void *))
{
	arm_intr_func.intr_establish	= intr_establish;
	arm_intr_func.intr_disestablish	= intr_disestablish;
	arm_intr_func.intr_string	= intr_string;
	arm_intr_dispatch		= intr_handle;
}

void arm_dflt_delay(u_int usecs);

struct {
	void	(*delay)(u_int);
	void	(*initclocks)(void);
	void	(*setstatclockrate)(int);
	void	(*mpstartclock)(void);
} arm_clock_func = {
	arm_dflt_delay,
	NULL,
	NULL,
	NULL
};

void
arm_clock_register(void (*initclock)(void), void (*delay)(u_int),
    void (*statclock)(int), void(*mpstartclock)(void))
{
	arm_clock_func.initclocks = initclock;
	arm_clock_func.delay = delay;
	arm_clock_func.setstatclockrate = statclock;
	arm_clock_func.mpstartclock = mpstartclock;
}


void
delay(u_int usec)
{
	arm_clock_func.delay(usec);
}

void
cpu_initclocks(void)
{
	if (arm_clock_func.initclocks == NULL)
		panic("initclocks function not initialized yet");

	arm_clock_func.initclocks();
}

void
arm_dflt_delay(u_int usecs)
{
	int j;
	/* BAH - there is no good way to make this close */
	/* but this isn't supposed to be used after the real clock attaches */
	for (; usecs > 0; usecs--)
		for (j = 100; j > 0; j--)
			;

}

todr_chip_handle_t todr_handle;

/*
 * inittodr:
 *
 *      Initialize time from the time-of-day register.
 */
#define MINYEAR         2003    /* minimum plausible year */
void
inittodr(time_t base)
{
	time_t deltat;
	struct timeval rtctime;
	struct timespec ts;
	int badbase;

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR;
		badbase = 1;
	} else
		badbase = 0;

	if (todr_handle == NULL ||
	    todr_gettime(todr_handle, &rtctime) != 0 ||
	    rtctime.tv_sec == 0) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		rtctime.tv_sec = base;
		rtctime.tv_usec = 0;
		if (todr_handle != NULL && !badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		ts.tv_sec = rtctime.tv_sec;
		ts.tv_nsec = rtctime.tv_usec * 1000;
		tc_setclock(&ts);
		goto bad;
	} else {
		ts.tv_sec = rtctime.tv_sec;
		ts.tv_nsec = rtctime.tv_usec * 1000;
		tc_setclock(&ts);
	}

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days; if
		 * so, assume something is amiss.
		 */
		deltat = rtctime.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;         /* all is well */
		printf("WARNING: clock %s %ld days\n",
		    rtctime.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
 bad:
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * resettodr:
 *
 *      Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{
	struct timeval rtctime;

	if (rtctime.tv_sec == 0)
		return;

	microtime(&rtctime);

	if (todr_handle != NULL &&
	   todr_settime(todr_handle, &rtctime) != 0)
		printf("resettodr: failed to set time\n");
}

void
setstatclockrate(int new)
{
	if (arm_clock_func.setstatclockrate == NULL) {
		panic("arm_clock_func.setstatclockrate not intialized");
	}
	arm_clock_func.setstatclockrate(new);
}

int
splraise(int s)
{
	return 0;
}

int
spllower(int s)
{
	return 0;
}

void
splx(int s)
{
}
