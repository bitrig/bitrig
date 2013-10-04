/* $OpenBSD: ampintc.c,v 1.3 2014/07/12 18:44:41 tedu Exp $ */
/*
 * Copyright (c) 2007,2009,2011 Dale Rahn <drahn@openbsd.org>
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

/*
 * This driver implements the interrupt controller as specified in 
 * DDI0407E_cortex_a9_mpcore_r2p0_trm with the 
 * IHI0048A_gic_architecture_spec_v1_0 underlying specification
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/ithread.h>
#include <sys/evcount.h>
#include <sys/proc.h>
#include <arm/cpufunc.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cortex/cortex.h>
#include <machine/fdt.h>

/* offset from periphbase */
#define ICP_ADDR	0x100
#define ICP_SIZE	0x100
#define ICD_ADDR	0x1000
#define ICD_SIZE	0x1000

#define	ICD_A7_A15_ADDR	0x1000
#define	ICD_A7_A15_SIZE	0x1000
#define	ICP_A7_A15_ADDR	0x2000
#define	ICP_A7_A15_SIZE	0x1000

/* registers */
#define	ICD_DCR			0x000
#define		ICD_DCR_ES		0x00000001
#define		ICD_DCR_ENS		0x00000002

#define ICD_ICTR			0x004
#define		ICD_ICTR_LSPI_SH	11
#define		ICD_ICTR_LSPI_M		0x1f
#define		ICD_ICTR_CPU_SH		5
#define		ICD_ICTR_CPU_M		0x07
#define		ICD_ICTR_ITL_SH		0
#define		ICD_ICTR_ITL_M		0x1f
#define ICD_IDIR			0x008
#define 	ICD_DIR_PROD_SH		24
#define 	ICD_DIR_PROD_M		0xff
#define 	ICD_DIR_REV_SH		12
#define 	ICD_DIR_REV_M		0xfff
#define 	ICD_DIR_IMP_SH		0
#define 	ICD_DIR_IMP_M		0xfff

#define IRQ_TO_REG32(i)		(((i) >> 5) & 0x7)
#define IRQ_TO_REG32BIT(i)	((i) & 0x1f)
#define IRQ_TO_REG4(i)		(((i) >> 2) & 0x3f)
#define IRQ_TO_REG4BIT(i)	((i) & 0x3)
#define IRQ_TO_REG16(i)		(((i) >> 4) & 0xf)
#define IRQ_TO_REGBIT_S(i)	8
#define IRQ_TO_REG4BIT_M(i)	8

#define ICD_ISRn(i)		(0x080 + (IRQ_TO_REG32(i) * 4))
#define ICD_ISERn(i)		(0x100 + (IRQ_TO_REG32(i) * 4))
#define ICD_ICERn(i)		(0x180 + (IRQ_TO_REG32(i) * 4))
#define ICD_ISPRn(i)		(0x200 + (IRQ_TO_REG32(i) * 4))
#define ICD_ICPRn(i)		(0x280 + (IRQ_TO_REG32(i) * 4))
#define ICD_ABRn(i)		(0x300 + (IRQ_TO_REG32(i) * 4))
#define ICD_IPRn(i)		(0x400 + (i))
#define ICD_IPTRn(i)		(0x800 + (i))
#define ICD_ICRn(i)		(0xC00 + (IRQ_TO_REG16(i) * 4))
/*
 * what about (ppi|spi)_status
 */
#define ICD_PPI			0xD00
#define 	ICD_PPI_GTIMER	(1 << 11)
#define 	ICD_PPI_FIQ		(1 << 12)
#define 	ICD_PPI_PTIMER	(1 << 13)
#define 	ICD_PPI_PWDOG	(1 << 14)
#define 	ICD_PPI_IRQ		(1 << 15)
#define ICD_SPI_BASE		0xD04
#define ICD_SPIn(i)			(ICD_SPI_BASE + ((i) * 4))


#define ICD_SGIR			0xF00

#define ICD_PERIPH_ID_0			0xFD0
#define ICD_PERIPH_ID_1			0xFD4
#define ICD_PERIPH_ID_2			0xFD8
#define ICD_PERIPH_ID_3			0xFDC
#define ICD_PERIPH_ID_4			0xFE0
#define ICD_PERIPH_ID_5			0xFE4
#define ICD_PERIPH_ID_6			0xFE8
#define ICD_PERIPH_ID_7			0xFEC

#define ICD_COMP_ID_0			0xFEC
#define ICD_COMP_ID_1			0xFEC
#define ICD_COMP_ID_2			0xFEC
#define ICD_COMP_ID_3			0xFEC

#define ICD_SIZE 			0x1000


#define ICPICR				0x00
#define ICPIPMR				0x04
/* XXX - must left justify bits to  0 - 7  */
#define 	ICMIPMR_SH 		4
#define ICPBPR				0x08
#define ICPIAR				0x0C
#define 	ICPIAR_IRQ_SH		0
#define 	ICPIAR_IRQ_M		0x3ff
#define 	ICPIAR_CPUID_SH		10
#define 	ICPIAR_CPUID_M		0x7
#define 	ICPIAR_NO_PENDING_IRQ	ICPIAR_IRQ_M
#define ICPEOIR				0x10
#define ICPPRP				0x14
#define ICPHPIR				0x18
#define ICPIIR				0xFC
#define ICP_SIZE			0x100
/*
 * what about periph_id and component_id 
 */

#define AMPAMPINTC_SIZE			0x1000


#define IRQ_ENABLE	1
#define IRQ_DISABLE	0

struct ampintc_softc {
	struct device		 sc_dev;
	struct intrsource	*sc_ampintc_handler;
	int			 sc_nintr;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_d_ioh, sc_p_ioh;
	struct evcount		 sc_spur;
};
struct ampintc_softc *ampintc;


int		 ampintc_match(struct device *, void *, void *);
void		 ampintc_attach(struct device *, struct device *, void *);
void		 ampintc_calc_mask(void);
void		*ampintc_intr_establish(int, int, int (*)(void *), void *,
		    char *);
void		*ampintc_intr_establish_ext(int, int, int (*)(void *), void *,
		    char *);
void		 ampintc_intr_disestablish(void *);
void		 ampintc_irq_handler(void *);
const char	*ampintc_intr_string(void *);
uint32_t	 ampintc_iack(void);
void		 ampintc_eoi(uint32_t);
void		 ampintc_set_priority(int, int);
void		 ampintc_intr_enable(int);
void		 ampintc_intr_disable(int);
void		 ampintc_route(int, int , int);
void		 ampintc_pic_hwunmask(struct pic *, int);

struct cfattach	ampintc_ca = {
	sizeof (struct ampintc_softc), ampintc_match, ampintc_attach
};

struct cfdriver ampintc_cd = {
	NULL, "ampintc", DV_DULL
};

static char *ampintc_compatibles[] = {
	"arm,gic",
	"arm,cortex-a7-gic",
	"arm,cortex-a9-gic",
	"arm,cortex-a15-gic",
	NULL
};

int
ampintc_match(struct device *parent, void *cfdata, void *aux)
{
	return (1);
}

void
ampintc_attach(struct device *parent, struct device *self, void *args)
{
	struct ampintc_softc *sc = (struct ampintc_softc *)self;
	struct cortex_attach_args *ia = args;
	int i, nintr;
	bus_space_tag_t		iot;
	bus_space_handle_t	d_ioh, p_ioh;
	void *node = NULL;
	uint32_t icp, icpsize, icd, icdsize;

	ampintc = sc;

	iot = ia->ca_iot;
	icp = ia->ca_periphbase + ICP_ADDR;
	icpsize = ICP_SIZE;
	icd = ia->ca_periphbase + ICD_ADDR;
	icdsize = ICD_SIZE;

	if ((cputype & CPU_ID_CORTEX_A7_MASK) == CPU_ID_CORTEX_A7 ||
	    (cputype & CPU_ID_CORTEX_A15_MASK) == CPU_ID_CORTEX_A15) {
		icp = ia->ca_periphbase + ICP_A7_A15_ADDR;
		icpsize = ICP_A7_A15_SIZE;
		icd = ia->ca_periphbase + ICD_A7_A15_ADDR;
		icdsize = ICD_A7_A15_SIZE;
	}

	for (i = 0; node == NULL && ampintc_compatibles[i] != NULL; i++)
		node = fdt_find_compatible(ampintc_compatibles[i]);
	if (node != NULL) {
		uint32_t reg[4];
		if (fdt_node_property_ints(node, "reg", reg, 4) == 4) {
			icd = reg[0];
			icdsize = reg[1];
			icp = reg[2];
			icpsize = reg[3];
		}
	}

	if (bus_space_map(iot, icp, icpsize, 0, &p_ioh))
		panic("ampintc_attach: ICP bus_space_map failed!");

	if (bus_space_map(iot, icd, icdsize, 0, &d_ioh))
		panic("ampintc_attach: ICD bus_space_map failed!");

	sc->sc_iot = iot;
	sc->sc_d_ioh = d_ioh;
	sc->sc_p_ioh = p_ioh;

	evcount_attach(&sc->sc_spur, "irq1023/spur", NULL);

	nintr = 32 * (bus_space_read_4(iot, d_ioh, ICD_ICTR) & ICD_ICTR_ITL_M);
	nintr += 32; /* ICD_ICTR + 1, irq 0-31 is SGI, 32+ is PPI */
	sc->sc_nintr = nintr;
	printf(" nirq %d\n", nintr);


	/* Disable all interrupts, clear all pending */
	for (i = 0; i < nintr/32; i++) {
		bus_space_write_4(iot, d_ioh, ICD_ICERn(i*32), ~0);
		bus_space_write_4(iot, d_ioh, ICD_ICPRn(i*32), ~0);
	}
	for (i = 0; i < nintr; i++) {
		/* lowest priority ?? */
		bus_space_write_1(iot, d_ioh, ICD_IPRn(i), 0xff);
		/* target no cpus */
		bus_space_write_1(iot, d_ioh, ICD_IPTRn(i), 0);
	}
	for (i = 2; i < nintr/16; i++) {
		/* irq 32 - N */
		bus_space_write_4(iot, d_ioh, ICD_ICRn(i*16), 0);
	}

	/* software reset of the part? */
	/* set protection bit (kernel only)? */

	/* XXX - check power saving bit */


	sc->sc_ampintc_handler = malloc(
	    (sizeof (*sc->sc_ampintc_handler) * nintr),
	    M_DEVBUF, M_ZERO | M_NOWAIT);

	struct pic *pic = malloc(sizeof(struct pic),
	    M_DEVBUF, M_ZERO | M_NOWAIT);
	pic->pic_hwunmask = ampintc_pic_hwunmask;

	for (i = 0; i < nintr; i++) {
		sc->sc_ampintc_handler[i].is_pin = i;
		sc->sc_ampintc_handler[i].is_pic = pic;
	}

	/* Set priority to highest, every IRQ will run there. */
	bus_space_write_4(sc->sc_iot, sc->sc_p_ioh, ICPIPMR, 0 << ICMIPMR_SH);
	ampintc_calc_mask();

	/* insert self as interrupt handler */
	arm_set_intr_handler(ampintc_intr_establish_ext,
	    ampintc_intr_disestablish, ampintc_intr_string, ampintc_irq_handler);

	/* enable interrupts */
	bus_space_write_4(iot, d_ioh, ICD_DCR, 3);
	bus_space_write_4(iot, p_ioh, ICPICR, 1);
	enable_interrupts(I32_bit);
}

void
ampintc_set_priority(int irq, int pri)
{
	struct ampintc_softc	*sc = ampintc;
	uint32_t		 prival;

	/*
	 * We only use 16 (13 really) interrupt priorities, 
	 * and a CPU is only required to implement bit 4-7 of each field
	 * so shift into the top bits.
	 * also low values are higher priority thus IPL_HIGH - pri
	 */
	prival = pri << ICMIPMR_SH;
	bus_space_write_1(sc->sc_iot, sc->sc_d_ioh, ICD_IPRn(irq), prival);
}

void
ampintc_intr_enable(int irq)
{
	struct ampintc_softc	*sc = ampintc;

#ifdef DEBUG
	printf("enable irq %d register %x bitmask %08x\n",
	    irq, ICD_ISERn(irq), 1 << IRQ_TO_REG32BIT(irq));
#endif

	bus_space_write_4(sc->sc_iot, sc->sc_d_ioh, ICD_ISERn(irq),
	    1 << IRQ_TO_REG32BIT(irq));
}

void
ampintc_intr_disable(int irq)
{
	struct ampintc_softc	*sc = ampintc;

	bus_space_write_4(sc->sc_iot, sc->sc_d_ioh, ICD_ICERn(irq),
	    1 << IRQ_TO_REG32BIT(irq));
}


void
ampintc_calc_mask(void)
{
	struct ampintc_softc	*sc = ampintc;
	int			 irq;

	for (irq = 0; irq < sc->sc_nintr; irq++) {
		if (sc->sc_ampintc_handler[irq].is_handlers != NULL) {
			ampintc_set_priority(irq, 0);
			ampintc_intr_enable(irq);
			ampintc_route(irq, IRQ_ENABLE, 0);
		} else {
			ampintc_intr_disable(irq);
			ampintc_route(irq, IRQ_DISABLE, 0);
		}
	}
}

uint32_t
ampintc_iack(void)
{
	uint32_t intid;
	struct ampintc_softc	*sc = ampintc;

	intid = bus_space_read_4(sc->sc_iot, sc->sc_p_ioh, ICPIAR);

	return (intid);
}

void
ampintc_eoi(uint32_t eoi)
{
	struct ampintc_softc	*sc = ampintc;

	bus_space_write_4(sc->sc_iot, sc->sc_p_ioh, ICPEOIR, eoi);
}

void
ampintc_route(int irq, int enable, int cpu)
{
	uint8_t  val;
	struct ampintc_softc	*sc = ampintc;

	val = bus_space_read_1(sc->sc_iot, sc->sc_d_ioh, ICD_IPTRn(irq));
	if (enable == IRQ_ENABLE)
		val |= (1 << cpu);
	else
		val &= ~(1 << cpu);
	bus_space_write_1(sc->sc_iot, sc->sc_d_ioh, ICD_IPTRn(irq), val);
}

void
ampintc_irq_handler(void *frame)
{
	struct ampintc_softc	*sc = ampintc;
	struct intrhand		*ih;
	void			*arg;
	uint32_t		 iack_val;
	int			 irq, pri;

	iack_val = ampintc_iack();
#define DEBUG_INTC
#ifdef DEBUG_INTC
	if (iack_val != 27)
	printf("irq  %d fired\n", iack_val);
	else {
		static int cnt = 0;
		if ((cnt++ % 100) == 0) {
			printf("irq  %d fired * _100\n", iack_val);
			Debugger();
		}

	}
#endif

	if (iack_val == 1023) {
		sc->sc_spur.ec_count++;
		return;
	}
	irq = iack_val & ((1 << sc->sc_nintr) - 1);

	pri = sc->sc_ampintc_handler[irq].is_maxlevel;
	crit_enter();
	if (sc->sc_ampintc_handler[irq].is_flags & IPL_DIRECT) {
		for (ih = sc->sc_ampintc_handler[irq].is_handlers; ih != NULL; ih = ih->ih_next) {
			if (ih->ih_arg != 0)
				arg = ih->ih_arg;
			else
				arg = frame;

			if (ih->ih_fun(arg))
				ih->ih_count.ec_count++;
		}
		/* ack it now */
		ampintc_eoi(iack_val);
	} else {
		ithread_run(&sc->sc_ampintc_handler[irq]);
		/* ack done after actual execution */
	}
	crit_leave();
}

void *
ampintc_intr_establish_ext(int irqno, int level, int (*fun)(void *),
    void *arg, char *name)
{
	return ampintc_intr_establish(irqno+32, level, fun, arg, name);
}

void *
ampintc_intr_establish(int irqno, int level, int (*fun)(void *),
    void *arg, char *name)
{
	struct ampintc_softc	*sc = ampintc;
	struct intrhand		*ih;
	int			 psw;

	if (irqno < 0 || irqno >= sc->sc_nintr)
		panic("ampintc_intr_establish: bogus irqnumber %d: %s",
		     irqno, name);

	/* no point in sleeping unless someone can free memory. */
	ih = (struct intrhand *)malloc (sizeof *ih, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");
	ih->ih_fun = fun;
	ih->ih_arg = arg;
	ih->ih_level = level & ~IPL_FLAGS;
	ih->ih_flags = level & IPL_FLAGS;
	ih->ih_irq = irqno;
	ih->ih_name = name;
	ih->ih_next = NULL;

	psw = disable_interrupts(I32_bit);

	/* XXX: add flags to interrupt source? */
	sc->sc_ampintc_handler[irqno].is_flags |= ih->ih_flags;

	if (sc->sc_ampintc_handler[irqno].is_handlers != NULL) {
		struct intrhand		*tmp;
		for (tmp = sc->sc_ampintc_handler[irqno].is_handlers; tmp->ih_next != NULL; tmp = tmp->ih_next);
		tmp->ih_next = ih;
	} else {
		sc->sc_ampintc_handler[irqno].is_handlers = ih;
	}

	if (!(ih->ih_flags & IPL_DIRECT))
		ithread_register(&sc->sc_ampintc_handler[irqno]);

	if (name != NULL)
		evcount_attach(&ih->ih_count, name, &ih->ih_irq);

#ifdef DEBUG_INTC
	printf("ampintc_intr_establish irq %d level %d [%s]\n", irqno, level,
	    name);
#endif
	ampintc_calc_mask();
	
	restore_interrupts(psw);
	return (ih);
}

void
ampintc_intr_disestablish(void *cookie)
{
#if 0
	int psw;
	struct intrhand *ih = cookie;
	int irqno = ih->ih_irq;
	psw = disable_interrupts(I32_bit);
	TAILQ_REMOVE(&sc->sc_ampintc_handler[irqno].iq_list, ih, ih_list);
	if (ih->ih_name != NULL)
		evcount_detach(&ih->ih_count);
	free(ih, M_DEVBUF, 0);
	restore_interrupts(psw);
#endif
}

const char *
ampintc_intr_string(void *cookie)
{
	struct intrhand *ih = (struct intrhand *)cookie;
	static char irqstr[1 + sizeof("ampintc irq ") + 4];

	snprintf(irqstr, sizeof irqstr, "ampintc irq %d", ih->ih_irq);
	return irqstr;
}

void
ampintc_pic_hwunmask(struct pic *pic, int pin)
{
	ampintc_eoi(pin);
}
