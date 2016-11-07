/*	$OpenBSD: intr.h,v 1.1 2013/09/04 14:38:27 patrick Exp $	*/
/*	$NetBSD: intr.h,v 1.12 2003/06/16 20:00:59 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MACHINE_INTR_H_
#define	_MACHINE_INTR_H_

#ifdef _KERNEL

/* Interrupt priority "levels". */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFT	1	/* generic software interrupts */
#define	IPL_SOFTCLOCK	2	/* software clock interrupt */
#define	IPL_SOFTNET	3	/* software network interrupt */
#define	IPL_SOFTTTY	4	/* software serial interrupt */
#define	IPL_BIO		5	/* block I/O */
#define	IPL_NET		6	/* network */
#define	IPL_TTY		7	/* terminals */
#define	IPL_VM		8	/* memory allocation */
#define	IPL_AUDIO	9	/* audio device */
#define	IPL_CLOCK	10	/* clock interrupt */
#define	IPL_STATCLOCK	11	/* statistics clock interrupt */
#define	IPL_SCHED	12	/* everything */
#define	IPL_HIGH	12	/* everything */
#define	IPL_IPI		13	/* everything + IPI*/

#define	NIPL		14

/* Interrupt priority "flags". */
#define	IPL_MPSAFE	0	/* no "mpsafe" interrupts */

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#define	IST_LEVEL_LOW		IST_LEVEL
#define	IST_LEVEL_HIGH		4
#define	IST_EDGE_FALLING	IST_EDGE
#define	IST_EDGE_RISING		5
#define	IST_EDGE_BOTH		6

#ifndef _LOCORE
#include <sys/evcount.h>
#include <sys/device.h>
#include <sys/queue.h>

int     splraise(int);
int     spllower(int);
void    splx(int);

void	arm_do_pending_intr(int);
void	arm_set_intr_handler_fdt(void *node,
	void *(*intr_establish)(void *node, int idx, int level,
	    int (*func)(void *), void *cookie, char *name));
void	arm_set_intr_handler(int (*raise)(int), int (*lower)(int),
	void (*x)(int), void (*setipl)(int),
	void *(*intr_establish)(int irqno, int level, int (*func)(void *),
	    void *cookie, char *name),
	void (*intr_disestablish)(void *cookie),
	const char *(*intr_string)(void *cookie),
	void (*intr_handle)(void *));

struct arm_intr_func {
	int (*raise)(int);
	int (*lower)(int);
	void (*x)(int);
	void (*setipl)(int);
	void *(*intr_establish)(int irqno, int level, int (*func)(void *),
	    void *cookie, char *name);
	void (*intr_disestablish)(void *cookie);
	const char *(*intr_string)(void *cookie);
};

extern struct arm_intr_func arm_intr_func;

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_fun)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
	struct evcount	ih_count;	/* interrupt counter */
	char *ih_name;			/* device name */
};

struct intrsource {
	TAILQ_HEAD(, intrhand) is_list;	/* handler list */
	int is_irq;			/* IRQ to mask while handling */
};

#define splraise(cpl)		(arm_intr_func.raise(cpl))
#define _splraise(cpl)		(arm_intr_func.raise(cpl))
#define spllower(cpl)		(arm_intr_func.lower(cpl))
#define splx(cpl)		(arm_intr_func.x(cpl))

#define	splhigh()	splraise(IPL_HIGH)
#define	splsoft()	splraise(IPL_SOFT)
#define	splsoftclock()	splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	splraise(IPL_SOFTNET)
#define	splbio()	splraise(IPL_BIO)
#define	splnet()	splraise(IPL_NET)
#define	spltty()	splraise(IPL_TTY)
#define	splvm()		splraise(IPL_VM)
#define	splaudio()	splraise(IPL_AUDIO)
#define	splclock()	splraise(IPL_CLOCK)
#define	splstatclock()	splraise(IPL_STATCLOCK)

#define	spl0()		spllower(IPL_NONE)

#define	splsched()	splhigh()
#define	spllock()	splhigh()

void arm_init_smask(void); /* XXX */
extern uint32_t arm_smask[NIPL];
void arm_setsoftintr(int si);

#define _setsoftintr arm_setsoftintr

/*
 * intr_state_t api.
 */

typedef	int intr_state_t;
#define	intr_disable()	disable_interrupts()
#define	intr_enable()	enable_interrupts()
#define	intr_restore(x)	restore_interrupts(x)


#include <machine/softintr.h>

void *arm_intr_establish(int irqno, int level, int (*func)(void *),
    void *cookie, char *name);
void arm_intr_disestablish(void *cookie);
const char *arm_intr_string(void *cookie);

/* XXX - this is probably the wrong location for this */
void arm_clock_register(void (*)(void), void (*)(u_int), void (*)(int),
    void (*)(void));

#ifdef DIAGNOSTIC
/*
 * Although this function is implemented in MI code, it must be in this MD
 * header because we don't want this header to include MI includes.
 */
void splassert_fail(int, int, const char *);
extern int splassert_ctl;
void arm_splassert_check(int, const char *);
#define splassert(__wantipl) do {                               \
	if (splassert_ctl > 0) {                                \
		arm_splassert_check(__wantipl, __func__);    \
	}                                                       \
} while (0)
#define splsoftassert(wantipl) splassert(wantipl)
#else
#define splassert(wantipl)      do { /* nothing */ } while (0)
#define splsoftassert(wantipl)  do { /* nothing */ } while (0)
#endif

#endif /* ! _LOCORE */

#define ARM_IRQ_HANDLER arm_intr

#endif /* _KERNEL */

#endif	/* _MACHINE_INTR_H_ */

