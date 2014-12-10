/* $OpenBSD: intc.c,v 1.3 2014/07/12 18:44:41 tedu Exp $ */
/*
 * Copyright (c) 2007,2009 Dale Rahn <drahn@openbsd.org>
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
#include <sys/ithread.h>
#include <sys/evcount.h>
#include <machine/bus.h>
#include <armv7/armv7/armv7var.h>
#include "intc.h"

#define INTC_NUM_IRQ intc_nirq
#define INTC_NUM_BANKS (intc_nirq/32)
#define INTC_MAX_IRQ 128
#define INTC_MAX_BANKS (INTC_MAX_IRQ/32)

/* registers */
#define	INTC_REVISION		0x00	/* R */
#define	INTC_SYSCONFIG		0x10	/* RW */
#define		INTC_SYSCONFIG_AUTOIDLE		0x1
#define		INTC_SYSCONFIG_SOFTRESET	0x2
#define	INTC_SYSSTATUS		0x14	/* R */
#define		INTC_SYSSYSTATUS_RESETDONE	0x1
#define	INTC_SIR_IRQ		0x40	/* R */	
#define	INTC_SIR_FIQ		0x44	/* R */
#define	INTC_CONTROL		0x48	/* RW */
#define		INTC_CONTROL_NEWIRQ	0x1
#define		INTC_CONTROL_NEWFIQ	0x2
#define		INTC_CONTROL_GLOBALMASK	0x1
#define	INTC_PROTECTION		0x4c	/* RW */
#define		INTC_PROTECTION_PROT 1	/* only privileged mode */
#define	INTC_IDLE		0x50	/* RW */

#define INTC_IRQ_TO_REG(i)	(((i) >> 5) & 0x3)
#define INTC_IRQ_TO_REGi(i)	((i) & 0x1f)
#define	INTC_ITRn(i)		0x80+(0x20*i)+0x00	/* R */
#define	INTC_MIRn(i)		0x80+(0x20*i)+0x04	/* RW */
#define	INTC_CLEARn(i)		0x80+(0x20*i)+0x08	/* RW */
#define	INTC_SETn(i)		0x80+(0x20*i)+0x0c	/* RW */
#define	INTC_ISR_SETn(i)	0x80+(0x20*i)+0x10	/* RW */
#define	INTC_ISR_CLEARn(i)	0x80+(0x20*i)+0x14	/* RW */
#define INTC_PENDING_IRQn(i)	0x80+(0x20*i)+0x18	/* R */
#define INTC_PENDING_FIQn(i)	0x80+(0x20*i)+0x1c	/* R */

#define INTC_ILRn(i)		0x100+(4*i)
#define		INTC_ILR_IRQ	0x0		/* not of FIQ */
#define		INTC_ILR_FIQ	0x1
#define		INTC_ILR_PRIs(pri)	((pri) << 2)
#define		INTC_ILR_PRI(reg)	(((reg) >> 2) & 0x2f)
#define		INTC_MIN_PRI	63
#define		INTC_STD_PRI	32
#define		INTC_MAX_PRI	0

volatile int softint_pending;

struct intrsource *intc_handler;
u_int32_t intc_imask[INTC_MAX_BANKS];

bus_space_tag_t		intc_iot;
bus_space_handle_t	intc_ioh;
int			intc_nirq;

void	intc_attach(struct device *, struct device *, void *);
void	intc_calc_mask(void);

struct cfattach	intc_ca = {
	sizeof (struct device), NULL, intc_attach
};

struct cfdriver intc_cd = {
	NULL, "intc", DV_DULL
};

int intc_attached = 0;

void
intc_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	int i;
	u_int32_t rev;

	intc_iot = aa->aa_iot;
	if (bus_space_map(intc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &intc_ioh))
		panic("intc_attach: bus_space_map failed!");

	rev = bus_space_read_4(intc_iot, intc_ioh, INTC_REVISION);

	printf(" rev %d.%d\n", rev >> 4 & 0xf, rev & 0xf);

	/* software reset of the part? */
	/* set protection bit (kernel only)? */
#if 0
	bus_space_write_4(intc_iot, intc_ioh, INTC_PROTECTION,
	     INTC_PROTECTION_PROT);
#endif

	/* enable interface clock power saving mode */
	bus_space_write_4(intc_iot, intc_ioh, INTC_SYSCONFIG,
	    INTC_SYSCONFIG_AUTOIDLE);

	switch (board_id) {
	case BOARD_ID_AM335X_BEAGLEBONE:
		intc_nirq = 128;
		break;
	default:
		intc_nirq = 96;
		break;
	}

	intc_handler = malloc(sizeof(*intc_handler) * INTC_NUM_IRQ,
	    M_DEVBUF, M_ZERO | M_NOWAIT);

	struct pic *pic = malloc(sizeof(struct pic),
			M_DEVBUF, M_ZERO | M_NOWAIT);
	//pic->pic_hwunmask = intc_pic_hwunmask;

	for (i = 0; i < INTC_NUM_IRQ; i++) {
		intc_handler[i].is_pin = i;
		intc_handler[i].is_pic = pic;
	}

	/* mask all interrupts */
	for (i = 0; i < INTC_NUM_BANKS; i++)
		bus_space_write_4(intc_iot, intc_ioh, INTC_MIRn(i), 0xffffffff);

	for (i = 0; i < INTC_NUM_IRQ; i++)
		bus_space_write_4(intc_iot, intc_ioh, INTC_ILRn(i),
		    INTC_ILR_PRIs(INTC_MIN_PRI)|INTC_ILR_IRQ);

	intc_calc_mask();
	bus_space_write_4(intc_iot, intc_ioh, INTC_CONTROL,
	    INTC_CONTROL_NEWIRQ);

	intc_attached = 1;

	/* insert self as interrupt handler */
	arm_set_intr_handler(intc_intr_establish, intc_intr_disestablish,
	    intc_intr_string, intc_irq_handler);

	enable_interrupts(I32_bit);
}

void
intc_calc_mask(void)
{
	int i, irq;

	for (irq = 0; irq < INTC_NUM_IRQ; irq++) {
		if (intc_handler[irq].is_handlers != NULL)
			intc_imask[irq] &= ~(1 << INTC_IRQ_TO_REGi(irq));
		else
			intc_imask[irq] |= (1 << INTC_IRQ_TO_REGi(irq));

		/* XXX - set enable/disable, priority */ 
		bus_space_write_4(intc_iot, intc_ioh, INTC_ILRn(irq),
		    INTC_ILR_PRIs(0)|INTC_ILR_IRQ);
	}

	for (i = 0; i < INTC_NUM_BANKS; i++)
		bus_space_write_4(intc_iot, intc_ioh,
		    INTC_MIRn(i), intc_imask[i]);

	bus_space_write_4(intc_iot, intc_ioh, INTC_CONTROL,
	    INTC_CONTROL_NEWIRQ);
}

void
intc_irq_handler(void *frame)
{
	int irq;
	struct intrhand *ih;
	void *arg;

	irq = bus_space_read_4(intc_iot, intc_ioh, INTC_SIR_IRQ);
#ifdef DEBUG_INTC
	printf("irq %d fired\n", irq);
#endif

	crit_enter();
	if (intc_handler[irq].is_flags & IPL_DIRECT) {
		for (ih = intc_handler[irq].is_handlers; ih != NULL; ih = ih->ih_next) {
			if (ih->ih_arg != 0)
				arg = ih->ih_arg;
			else
				arg = frame;

			if (ih->ih_fun(arg))
				ih->ih_count.ec_count++;
		}
		/* ack it now */
		//ampintc_eoi(iack_val);
	} else {
		ithread_run(&intc_handler[irq]);
		/* ack done after actual execution */
	}
	bus_space_write_4(intc_iot, intc_ioh, INTC_CONTROL,
	    INTC_CONTROL_NEWIRQ);
	crit_leave();
}

void *
intc_intr_establish(int irqno, int level, int (*func)(void *),
    void *arg, char *name)
{
	int psw;
	struct intrhand *ih;

	if (irqno < 0 || irqno >= INTC_NUM_IRQ)
		panic("intc_intr_establish: bogus irqnumber %d: %s",
		     irqno, name);
	psw = disable_interrupts(I32_bit);

	/* no point in sleeping unless someone can free memory. */
	ih = (struct intrhand *)malloc (sizeof *ih, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");
	ih->ih_fun = func;
	ih->ih_arg = arg;
	ih->ih_level = level & ~IPL_FLAGS;
	ih->ih_flags = level & IPL_FLAGS;
	ih->ih_irq = irqno;
	ih->ih_name = name;

	if (intc_handler[irqno].is_handlers != NULL) {
		struct intrhand *tmp;
		for (tmp = intc_handler[irqno].is_handlers; tmp->ih_next != NULL; tmp = tmp->ih_next);
		tmp->ih_next = ih;
	} else {
		intc_handler[irqno].is_handlers = ih;
	}

	if (!(ih->ih_flags & IPL_DIRECT))
		ithread_register(&intc_handler[irqno]);

	if (name != NULL)
		evcount_attach(&ih->ih_count, name, &ih->ih_irq);

#ifdef DEBUG_INTC
	printf("intc_intr_establish irq %d level %d [%s]\n", irqno, level,
	    name);
#endif
	intc_calc_mask();
	
	restore_interrupts(psw);
	return (ih);
}

void
intc_intr_disestablish(void *cookie)
{
	int psw;
	struct intrhand *ih = cookie;
	//int irqno = ih->ih_irq;
	psw = disable_interrupts(I32_bit);
	//TAILQ_REMOVE(&intc_handler[irqno].iq_list, ih, ih_list);
	if (ih->ih_name != NULL)
		evcount_detach(&ih->ih_count);
	free(ih, M_DEVBUF, 0);
	restore_interrupts(psw);
}

const char *
intc_intr_string(void *cookie)
{
	return "huh?";
}


#if 0
int intc_tst(void *a);

int
intc_tst(void *a)
{
	printf("inct_tst called\n");
	bus_space_write_4(intc_iot, intc_ioh, INTC_ISR_CLEARn(0), 2);
	return 1;
}

void intc_test(void);
void intc_test(void)
{
	void * ih;
	printf("about to register handler\n");
	ih = intc_intr_establish(1, IPL_BIO, intc_tst, NULL, "intctst");

	printf("about to set bit\n");
	bus_space_write_4(intc_iot, intc_ioh, INTC_ISR_SETn(0), 2);

	printf("about to clear bit\n");
	bus_space_write_4(intc_iot, intc_ioh, INTC_ISR_CLEARn(0), 2);

	printf("about to remove handler\n");
	intc_intr_disestablish(ih);

	printf("done\n");
}
#endif
