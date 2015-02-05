/*
 * Copyright (c) 2007,2009 Dale Rahn <drahn@openbsd.org>
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
#include <sys/evcount.h>
#include <machine/bus.h>
#include <machine/fdt.h>
#include <arm/cpufunc.h>
#include <armv7/armv7/armv7var.h>

/* registers */
#define	INTC_PENDING_BANK0	0x00
#define	INTC_PENDING_BANK1	0x04
#define	INTC_PENDING_BANK2	0x08
#define	INTC_FIQ_CONTROL	0x0C
#define	INTC_ENABLE_BANK1	0x10
#define	INTC_ENABLE_BANK2	0x14
#define	INTC_ENABLE_BANK0	0x18
#define	INTC_DISABLE_BANK1	0x1C
#define	INTC_DISABLE_BANK2	0x20
#define	INTC_DISABLE_BANK0	0x24

/* arm local */
#define	ARM_LOCAL_CONTROL		0x00
#define	ARM_LOCAL_PRESCALER		0x08
#define	 PRESCALER_19_2			0x80000000 /* 19.2 MHz */
#define	ARM_LOCAL_INT_TIMER(n)		(0x40 + (n) * 4)
#define	ARM_LOCAL_INT_MAILBOX(n)	(0x50 + (n) * 4)
#define	ARM_LOCAL_INT_PENDING(n)	(0x60 + (n) * 4)
#define	 ARM_LOCAL_INT_PENDING_MASK	0x0f

#define	BANK0_START	64
#define	BANK0_END	(BANK0_START + 32 - 1)
#define	BANK1_START	0
#define	BANK1_END	(BANK1_START + 32 - 1)
#define	BANK2_START	32
#define	BANK2_END	(BANK2_START + 32 - 1)
#define	LOCAL_START	96
#define	LOCAL_END	(LOCAL_START + 32 - 1)

#define	IS_IRQ_BANK0(n)	(((n) >= BANK0_START) && ((n) <= BANK0_END))
#define	IS_IRQ_BANK1(n)	(((n) >= BANK1_START) && ((n) <= BANK1_END))
#define	IS_IRQ_BANK2(n)	(((n) >= BANK2_START) && ((n) <= BANK2_END))
#define	IS_IRQ_LOCAL(n)	(((n) >= LOCAL_START) && ((n) <= LOCAL_END))
#define	IRQ_BANK0(n)	((n) - BANK0_START)
#define	IRQ_BANK1(n)	((n) - BANK1_START)
#define	IRQ_BANK2(n)	((n) - BANK2_START)
#define	IRQ_LOCAL(n)	((n) - LOCAL_START)

#define	INTC_NIRQ	128
#define	INTC_NBANK	4

#define INTC_IRQ_TO_REG(i)	(((i) >> 5) & 0x3)
#define INTC_IRQ_TO_REGi(i)	((i) & 0x1f)

struct bcm_intc_softc {
	struct device		 sc_dev;
	struct intrsource	 sc_bcm_intc_handler[INTC_NIRQ];
	uint32_t		 sc_bcm_intc_imask[INTC_NBANK][NIPL];
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	bus_space_handle_t	 sc_lioh;
	int			 sc_ncells;
};
struct bcm_intc_softc *bcm_intc;

int	 bcm_intc_match(struct device *, void *, void *);
void	 bcm_intc_attach(struct device *, struct device *, void *);
void	 bcm_intc_splx(int new);
int	 bcm_intc_spllower(int new);
int	 bcm_intc_splraise(int new);
void	 bcm_intc_setipl(int new);
void	 bcm_intc_calc_mask(void);
void	*bcm_intc_intr_establish(int, int, int (*)(void *),
    void *, char *);
void	*bcm_intc_intr_establish_fdt_idx(void *, int, int, int (*)(void *),
    void *, char *);
void	 bcm_intc_intr_disestablish(void *);
const char *bcm_intc_intr_string(void *);
void	 bcm_intc_irq_handler(void *);

struct cfattach	bcmintc_ca = {
	sizeof (struct bcm_intc_softc), bcm_intc_match, bcm_intc_attach
};

struct cfdriver bcmintc_cd = {
	NULL, "bcmintc", DV_DULL
};

int
bcm_intc_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("brcm,bcm2835-armctrl-ic", aa->aa_node) ||
	    fdt_node_compatible("broadcom,bcm2835-armctrl-ic", aa->aa_node))
		return (1);

	return 0;
}

void
bcm_intc_attach(struct device *parent, struct device *self, void *args)
{
	struct bcm_intc_softc *sc = (struct bcm_intc_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	int i;

	bcm_intc = sc;

	sc->sc_iot = aa->aa_iot;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT",
		    __func__);

	if (fdt_node_property_int(aa->aa_node, "#interrupt-cells",
	    &sc->sc_ncells) != 1)
		panic("%s: no #interrupt-cells property", sc->sc_dev.dv_xname);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	/*
	 * ARM Local area, because fuck you.
	 *
	 * The original rPi with the BCM2835 does implement the same IC
	 * but with a different IRQ basis. On the BCM2836 there's an
	 * additional area to enable Timer/Mailbox interrupts, which
	 * is not yet exposed in the given DT.
	 */
	if (bus_space_map(sc->sc_iot, 0x40000000, 0x1000, 0, &sc->sc_lioh))
		panic("%s: bus_space_map failed!", __func__);

	printf("\n");

	/* mask all interrupts */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK0,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK1,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK2,
	    0xffffffff);

	/* ARM local specific */
	bus_space_write_4(sc->sc_iot, sc->sc_lioh, ARM_LOCAL_CONTROL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_lioh, ARM_LOCAL_PRESCALER,
	    PRESCALER_19_2);
	for (int i = 0; i < 4; i++)
		bus_space_write_4(sc->sc_iot, sc->sc_lioh,
		    ARM_LOCAL_INT_TIMER(i), 0);
	for (int i = 0; i < 4; i++)
		bus_space_write_4(sc->sc_iot, sc->sc_lioh,
		    ARM_LOCAL_INT_MAILBOX(i), 1);

	for (i = 0; i < INTC_NIRQ; i++) {
		TAILQ_INIT(&sc->sc_bcm_intc_handler[i].is_list);
	}

	bcm_intc_calc_mask();

	/* insert self as interrupt handler */
	arm_set_intr_handler(bcm_intc_splraise, bcm_intc_spllower, bcm_intc_splx,
	    bcm_intc_setipl,
	    bcm_intc_intr_establish, bcm_intc_intr_disestablish, bcm_intc_intr_string,
	    bcm_intc_irq_handler);

	arm_set_intr_handler_fdt(aa->aa_node, bcm_intc_intr_establish_fdt_idx);

	bcm_intc_setipl(IPL_HIGH);  /* XXX ??? */
	intr_enable();
}

void
bcm_intc_intr_enable(int irq, int ipl)
{
	struct bcm_intc_softc	*sc = bcm_intc;

	if (IS_IRQ_BANK0(irq))
		sc->sc_bcm_intc_imask[0][ipl] |= (1 << IRQ_BANK0(irq));
	else if (IS_IRQ_BANK1(irq))
		sc->sc_bcm_intc_imask[1][ipl] |= (1 << IRQ_BANK1(irq));
	else if (IS_IRQ_BANK2(irq))
		sc->sc_bcm_intc_imask[2][ipl] |= (1 << IRQ_BANK2(irq));
	else if (IS_IRQ_LOCAL(irq))
		sc->sc_bcm_intc_imask[3][ipl] |= (1 << IRQ_LOCAL(irq));
	else
		printf("%s: invalid irq number: %d\n", __func__, irq);
}

void
bcm_intc_intr_disable(int irq, int ipl)
{
	struct bcm_intc_softc	*sc = bcm_intc;

	if (IS_IRQ_BANK0(irq))
		sc->sc_bcm_intc_imask[0][ipl] &= ~(1 << IRQ_BANK0(irq));
	else if (IS_IRQ_BANK1(irq))
		sc->sc_bcm_intc_imask[1][ipl] &= ~(1 << IRQ_BANK1(irq));
	else if (IS_IRQ_BANK2(irq))
		sc->sc_bcm_intc_imask[2][ipl] &= ~(1 << IRQ_BANK2(irq));
	else if (IS_IRQ_LOCAL(irq))
		sc->sc_bcm_intc_imask[3][ipl] &= ~(1 << IRQ_LOCAL(irq));
	else
		printf("%s: invalid irq number: %d\n", __func__, irq);
}

void
bcm_intc_calc_mask(void)
{
	struct cpu_info *ci = curcpu();
	struct bcm_intc_softc *sc = bcm_intc;
	int irq;
	struct intrhand *ih;
	int i;

	for (irq = 0; irq < INTC_NIRQ; irq++) {
		int max = IPL_NONE;
		int min = IPL_HIGH;
		TAILQ_FOREACH(ih, &sc->sc_bcm_intc_handler[irq].is_list,
		    ih_list) {
			if (ih->ih_ipl > max)
				max = ih->ih_ipl;

			if (ih->ih_ipl < min)
				min = ih->ih_ipl;
		}

		sc->sc_bcm_intc_handler[irq].is_irq = max;

		if (max == IPL_NONE)
			min = IPL_NONE;

#ifdef DEBUG_INTC
		if (min != IPL_NONE) {
			printf("irq %d to block at %d %d reg %d bit %d\n",
			    irq, max, min, INTC_IRQ_TO_REG(irq),
			    INTC_IRQ_TO_REGi(irq));
		}
#endif
		/* Enable interrupts at lower levels, clear -> enable */
		for (i = 0; i < min; i++)
			bcm_intc_intr_enable(irq, i);
		for (; i <= IPL_HIGH; i++)
			bcm_intc_intr_disable(irq, i);
	}
	arm_init_smask();
	bcm_intc_setipl(ci->ci_cpl);
}

void
bcm_intc_splx(int new)
{
	struct cpu_info *ci = curcpu();
	bcm_intc_setipl(new);

	if (ci->ci_ipending & arm_smask[ci->ci_cpl])
		arm_do_pending_intr(ci->ci_cpl);
}

int
bcm_intc_spllower(int new)
{
	struct cpu_info *ci = curcpu();
	int old = ci->ci_cpl;
	bcm_intc_splx(new);
	return (old);
}

int
bcm_intc_splraise(int new)
{
	struct cpu_info *ci = curcpu();
	int old;
	old = ci->ci_cpl;

	/*
	 * setipl must always be called because there is a race window
	 * where the variable is updated before the mask is set
	 * an interrupt occurs in that window without the mask always
	 * being set, the hardware might not get updated on the next
	 * splraise completely messing up spl protection.
	 */
	if (old > new)
		new = old;

	bcm_intc_setipl(new);

	return (old);
}

void
bcm_intc_setipl(int new)
{
	struct cpu_info *ci = curcpu();
	struct bcm_intc_softc *sc = bcm_intc;
	int psw;

	psw = intr_disable();
	ci->ci_cpl = new;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK0,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK1,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_DISABLE_BANK2,
	    0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_ENABLE_BANK0,
	    sc->sc_bcm_intc_imask[0][new]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_ENABLE_BANK1,
	    sc->sc_bcm_intc_imask[1][new]);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, INTC_ENABLE_BANK2,
	    sc->sc_bcm_intc_imask[2][new]);
	/* XXX: SMP */
	for (int i = 0; i < 4; i++)
		bus_space_write_4(sc->sc_iot, sc->sc_lioh,
		    ARM_LOCAL_INT_TIMER(i), sc->sc_bcm_intc_imask[3][new]);
	intr_restore(psw);
}

int
bcm_intc_get_next_irq(int last_irq)
{
	struct bcm_intc_softc *sc = bcm_intc;
	uint32_t pending;
	int32_t irq = last_irq + 1;

	/* Sanity check */
	if (irq < 0)
		irq = 0;

	/* We need to keep this order. */
	/* TODO: should we mask last_irq? */
	if (IS_IRQ_BANK1(irq)) {
		pending = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    INTC_PENDING_BANK1);
		if (pending == 0) {
			irq = BANK2_START;	/* skip to next bank */
		} else do {
			if (pending & (1 << IRQ_BANK1(irq)))
				return irq;
			irq++;
		} while (IS_IRQ_BANK1(irq));
	}
	if (IS_IRQ_BANK2(irq)) {
		pending = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    INTC_PENDING_BANK2);
		if (pending == 0) {
			irq = BANK0_START;	/* skip to next bank */
		} else do {
			if (pending & (1 << IRQ_BANK2(irq)))
				return irq;
			irq++;
		} while (IS_IRQ_BANK2(irq));
	}
	if (IS_IRQ_BANK0(irq)) {
		pending = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    INTC_PENDING_BANK0);
		if (pending == 0) {
			irq = LOCAL_START;	/* skip to next bank */
		} else do {
			if (pending & (1 << IRQ_BANK0(irq)))
				return irq;
			irq++;
		} while (IS_IRQ_BANK0(irq));
	}
	if (IS_IRQ_LOCAL(irq)) {
		/* XXX: SMP */
		pending = bus_space_read_4(sc->sc_iot, sc->sc_lioh,
		    ARM_LOCAL_INT_PENDING(0));
		pending &= ARM_LOCAL_INT_PENDING_MASK;
		if (pending != 0) do {
			if (pending & (1 << IRQ_LOCAL(irq)))
				return irq;
			irq++;
		} while (IS_IRQ_LOCAL(irq));
	}
	return (-1);
}

static void
bcm_intc_call_handler(int irq, void *frame)
{
	struct bcm_intc_softc *sc = bcm_intc;
	struct intrhand *ih;
	int pri, s;
	void *arg;

//#define DEBUG_INTC
#ifdef DEBUG_INTC
	if (irq != 99)
		printf("irq  %d fired\n", irq);
	else {
		static int cnt = 0;
		if ((cnt++ % 100) == 0) {
			printf("irq  %d fired * _100\n", irq);
			Debugger();
		}
	}
#endif

	pri = sc->sc_bcm_intc_handler[irq].is_irq;
	s = bcm_intc_splraise(pri);
	TAILQ_FOREACH(ih, &sc->sc_bcm_intc_handler[irq].is_list, ih_list) {
		if (ih->ih_arg != 0)
			arg = ih->ih_arg;
		else
			arg = frame;

		if (ih->ih_fun(arg))
			ih->ih_count.ec_count++;

	}

	bcm_intc_splx(s);
}

void
bcm_intc_irq_handler(void *frame)
{
	int irq = -1;

	while ((irq = bcm_intc_get_next_irq(irq)) != -1)
		bcm_intc_call_handler(irq, frame);
}

void *
bcm_intc_intr_establish_fdt_idx(void *node, int idx, int level,
    int (*func)(void *), void *arg, char *name)
{
	struct bcm_intc_softc	*sc = bcm_intc;
	int nints = sc->sc_ncells * (idx + 1);
	int ints[nints];
	int irq;

	if (fdt_node_property_ints(node, "interrupts", ints, nints) != nints)
		panic("%s: no interrupts property", sc->sc_dev.dv_xname);

	irq = ints[idx * 2 + 1];
	if (ints[idx * 2] == 0)
		irq += BANK0_START;
	else if (ints[idx * 2] == 1)
		irq += BANK1_START;
	else if (ints[idx * 2] == 2)
		irq += BANK2_START;
	else if (ints[idx * 2] == 3)
		irq += LOCAL_START;
	else
		panic("%s: bogus interrupt type", sc->sc_dev.dv_xname);

	return bcm_intc_intr_establish(irq, level, func, arg, name);
}

void *
bcm_intc_intr_establish(int irqno, int level, int (*func)(void *),
    void *arg, char *name)
{
	struct bcm_intc_softc *sc = bcm_intc;
	struct intrhand *ih;
	int psw;

	if (irqno < 0 || irqno >= INTC_NIRQ)
		panic("bcm_intc_intr_establish: bogus irqnumber %d: %s",
		     irqno, name);
	psw = intr_disable();

	/* no point in sleeping unless someone can free memory. */
	ih = (struct intrhand *)malloc (sizeof *ih, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");
	ih->ih_fun = func;
	ih->ih_arg = arg;
	ih->ih_ipl = level;
	ih->ih_irq = irqno;
	ih->ih_name = name;

	TAILQ_INSERT_TAIL(&sc->sc_bcm_intc_handler[irqno].is_list, ih, ih_list);

	if (name != NULL)
		evcount_attach(&ih->ih_count, name, &ih->ih_irq);

#ifdef DEBUG_INTC
	printf("%s irq %d level %d [%s]\n", __func__, irqno, level,
	    name);
#endif
	bcm_intc_calc_mask();

	intr_restore(psw);
	return (ih);
}

void
bcm_intc_intr_disestablish(void *cookie)
{
	struct bcm_intc_softc *sc = bcm_intc;
	struct intrhand *ih = cookie;
	int irqno = ih->ih_irq;
	int psw;
	psw = intr_disable();
	TAILQ_REMOVE(&sc->sc_bcm_intc_handler[irqno].is_list, ih, ih_list);
	if (ih->ih_name != NULL)
		evcount_detach(&ih->ih_count);
	free(ih, M_DEVBUF, 0);
	intr_restore(psw);
}

const char *
bcm_intc_intr_string(void *cookie)
{
	return "huh?";
}
