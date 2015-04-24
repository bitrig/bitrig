/*
 * Copyright (c) 2007,2009,2011 Dale Rahn <drahn@openbsd.org>
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
#include <arm/cpufunc.h>
#include <machine/bus.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>

/* registers */
#define	MPIC_CTRL		0x000 /* control register */
#define		MPIC_CTRL_PRIO_EN	0x1
#define	MPIC_SOFTINT		0x004 /* software triggered interrupt register */
#define	MPIC_INTERR		0x020 /* SOC main interrupt error cause register */
#define	MPIC_ISE		0x030 /* interrupt set enable */
#define	MPIC_ICE		0x034 /* interrupt clear enable */
#define	MPIC_ISCR(x)		(0x100 + (4 * x)) /* interrupt x source control register */
#define		MPIC_ISCR_PRIO_SHIFT	24
#define		MPIC_ISCR_INTEN		(1 << 28)

#define	MPIC_CTP		0x040 /* current task priority */
#define		MPIC_CTP_SHIFT		28
#define	MPIC_IACK		0x044 /* interrupt acknowledge */
#define	MPIC_ISM		0x048 /* set mask */
#define	MPIC_ICM		0x04C /* clear mask */

#define	MPIC_DOORBELL_CAUSE	0x08

struct mpic_softc {
	struct device		 sc_dev;
	struct intrsource 	*sc_mpic_handler;
	int			 sc_nintr;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_m_ioh, sc_c_ioh;
	struct evcount		 sc_spur;
	int			 sc_ncells;
};
struct mpic_softc *mpic;

int		 mpic_match(struct device *, void *, void *);
int		 mpic_fdt_match(struct device *, void *, void *);
void		 mpic_attach(struct device *, struct device *, void *);
int		 mpic_spllower(int);
void		 mpic_splx(int);
int		 mpic_splraise(int);
void		 mpic_setipl(int);
void		 mpic_calc_mask(void);
void		*mpic_intr_establish(int, int, int (*)(void *), void *,
		    char *);
void		*mpic_intr_establish_fdt_idx(void *, int, int,
		    int (*)(void *), void *, char *);
void		 mpic_intr_disestablish(void *);
void		 mpic_irq_handler(void *);
const char	*mpic_intr_string(void *);
void		 mpic_set_priority(int, int);
void		 mpic_intr_enable(int);
void		 mpic_intr_disable(int);

struct cfattach	mvmpic_ca = {
	sizeof (struct mpic_softc), mpic_match, mpic_attach
};

struct cfdriver mvmpic_cd = {
	NULL, "mvmpic", DV_DULL
};

int
mpic_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,mpic", aa->aa_node))
		return (1);

	return (0);
}

void
mpic_attach(struct device *parent, struct device *self, void *args)
{
	struct mpic_softc *sc = (struct mpic_softc *)self;
	struct armv7_attach_args *aa = args;
	uint32_t main, mainsize, cpu, cpusize;
	struct fdt_memory mem;
	int i;

	mpic = sc;

	arm_init_smask();

	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: cannot extract main memory",
		    sc->sc_dev.dv_xname);
	main = mem.addr;
	mainsize = mem.size;

	if (fdt_get_memory_address(aa->aa_node, 1, &mem))
		panic("%s: cannot extract cpu memory",
		    sc->sc_dev.dv_xname);
	cpu = mem.addr;
	cpusize = mem.size;

	if (fdt_node_property_int(aa->aa_node,
	    "#interrupt-cells", &sc->sc_ncells) != 1)
		panic("%s: no #interrupt-cells property",
		    sc->sc_dev.dv_xname);

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, main, mainsize, 0, &sc->sc_m_ioh))
		panic("%s: main bus_space_map failed!", __func__);

	if (bus_space_map(sc->sc_iot, cpu, cpusize, 0, &sc->sc_c_ioh))
		panic("%s: cpu bus_space_map failed!", __func__);

	evcount_attach(&sc->sc_spur, "irq1023/spur", NULL);

	sc->sc_nintr = (bus_space_read_4(sc->sc_iot, sc->sc_m_ioh,
	    MPIC_CTRL) >> 2) & 0x3ff;
	printf(" nirq %d\n", sc->sc_nintr);

	/* Disable all interrupts */
	for (i = 0; i < sc->sc_nintr; i++) {
		bus_space_write_4(sc->sc_iot, sc->sc_m_ioh, MPIC_ICE, i);
		bus_space_write_4(sc->sc_iot, sc->sc_c_ioh, MPIC_ISM, i);
	}

	/* Clear pending IPIs */
	bus_space_write_4(sc->sc_iot, sc->sc_c_ioh, MPIC_DOORBELL_CAUSE, 0);

	/* Enable hardware priorization selection */
	bus_space_write_4(sc->sc_iot, sc->sc_m_ioh, MPIC_CTRL,
	    MPIC_CTRL_PRIO_EN);

	sc->sc_mpic_handler = mallocarray(sc->sc_nintr,
	    sizeof(*sc->sc_mpic_handler), M_DEVBUF, M_ZERO | M_NOWAIT);
	for (i = 0; i < sc->sc_nintr; i++) {
		TAILQ_INIT(&sc->sc_mpic_handler[i].is_list);
	}

	mpic_setipl(IPL_HIGH);  /* XXX ??? */
	mpic_calc_mask();

	/* insert self as interrupt handler */
	arm_set_intr_handler(mpic_splraise, mpic_spllower, mpic_splx,
	    mpic_setipl, mpic_intr_establish, mpic_intr_disestablish,
	    mpic_intr_string, mpic_irq_handler);

	arm_set_intr_handler_fdt(aa->aa_node, mpic_intr_establish_fdt_idx);

	/* enable interrupts */
	intr_enable();
}

void
mpic_set_priority(int irq, int pri)
{
	struct mpic_softc	*sc = mpic;

	bus_space_write_4(sc->sc_iot, sc->sc_m_ioh, MPIC_ISCR(irq),
	    (bus_space_read_4(sc->sc_iot, sc->sc_m_ioh, MPIC_ISCR(irq)) &
	    ~(0xf << MPIC_ISCR_PRIO_SHIFT)) | (pri << MPIC_ISCR_PRIO_SHIFT));
}

void
mpic_setipl(int new)
{
	struct cpu_info		*ci = curcpu();
	struct mpic_softc	*sc = mpic;
	int			 psw;

	/* disable here is only to keep hardware in sync with ci->ci_cpl */
	psw = intr_disable();
	ci->ci_cpl = new;

	bus_space_write_4(sc->sc_iot, sc->sc_c_ioh, MPIC_CTP,
	    (bus_space_read_4(sc->sc_iot, sc->sc_c_ioh, MPIC_CTP) &
	    ~(0xf << MPIC_CTP_SHIFT)) | (new << MPIC_CTP_SHIFT));
	intr_restore(psw);
}

void
mpic_intr_enable(int irq)
{
	struct mpic_softc	*sc = mpic;

#ifdef DEBUG
	printf("enable irq %d\n", irq);
#endif

	bus_space_write_4(sc->sc_iot, sc->sc_m_ioh, MPIC_ISE, irq);
	bus_space_write_4(sc->sc_iot, sc->sc_c_ioh, MPIC_ICM, irq);
}

void
mpic_intr_disable(int irq)
{
	struct mpic_softc	*sc = mpic;

	bus_space_write_4(sc->sc_iot, sc->sc_m_ioh, MPIC_ICE, irq);
	bus_space_write_4(sc->sc_iot, sc->sc_c_ioh, MPIC_ISM, irq);
}


void
mpic_calc_mask(void)
{
	struct cpu_info		*ci = curcpu();
        struct mpic_softc	*sc = mpic;
	struct intrhand		*ih;
	int			 irq;

	for (irq = 0; irq < sc->sc_nintr; irq++) {
		int max = IPL_NONE;
		int min = IPL_HIGH;
		TAILQ_FOREACH(ih, &sc->sc_mpic_handler[irq].is_list,
		    ih_list) {
			if (ih->ih_ipl > max)
				max = ih->ih_ipl;

			if (ih->ih_ipl < min)
				min = ih->ih_ipl;
		}

		if (sc->sc_mpic_handler[irq].is_irq == max) {
			continue;
		}
		sc->sc_mpic_handler[irq].is_irq = max;

		if (max == IPL_NONE)
			min = IPL_NONE;

#ifdef DEBUG_INTC
		if (min != IPL_NONE) {
			printf("irq %d to block at %d %d\n", irq, max, min);
		}
#endif
		/* Enable interrupts at lower levels, clear -> enable */
		/* Set interrupt priority/enable */
		if (min != IPL_NONE) {
			mpic_set_priority(irq, min);
			mpic_intr_enable(irq);
		} else {
			mpic_intr_disable(irq);

		}
	}
	mpic_setipl(ci->ci_cpl);
}

void
mpic_splx(int new)
{
	struct cpu_info *ci = curcpu();

	if (ci->ci_ipending & arm_smask[new])
		arm_do_pending_intr(new);

	mpic_setipl(new);
}

int
mpic_spllower(int new)
{
	struct cpu_info *ci = curcpu();
	int old = ci->ci_cpl;
	mpic_splx(new);
	return (old);
}

int
mpic_splraise(int new)
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

	mpic_setipl(new);

	return (old);
}


void
mpic_irq_handler(void *frame)
{
	struct mpic_softc	*sc = mpic;
	struct intrhand		*ih;
	void			*arg;
	int			 irq, pri, s;

	irq = bus_space_read_4(sc->sc_iot, sc->sc_c_ioh, MPIC_IACK) & 0x3ff;
//#define DEBUG_INTC
#ifdef DEBUG_INTC
	if (irq != 5)
	printf("irq %d fired\n", irq);
	else {
		static int cnt = 0;
		if ((cnt++ % 100) == 0) {
			printf("irq %d fired * _100\n", irq);
			Debugger();
		}
	}
#endif

	if (irq == 1023) {
		sc->sc_spur.ec_count++;
		return;
	}

	if (irq >= sc->sc_nintr)
		return;

	pri = sc->sc_mpic_handler[irq].is_irq;
	s = mpic_splraise(pri);
	TAILQ_FOREACH(ih, &sc->sc_mpic_handler[irq].is_list, ih_list) {
		if (ih->ih_arg != 0)
			arg = ih->ih_arg;
		else
			arg = frame;

		if (ih->ih_fun(arg))
			ih->ih_count.ec_count++;

	}

	mpic_splx(s);
}

void *
mpic_intr_establish_fdt_idx(void *node, int idx, int level,
    int (*func)(void *), void *arg, char *name)
{
	struct mpic_softc	*sc = mpic;
	int			 nints = sc->sc_ncells * (idx + 1);
	int			 intr_elem = idx * sc->sc_ncells;
	int			 ints[nints];

	/* Load only parts needed from the interrupt property, not all. */
	if (fdt_node_property_ints(node, "interrupts", ints, nints) != nints)
		panic("%s: no interrupts property", sc->sc_dev.dv_xname);

	return mpic_intr_establish(ints[intr_elem], level, func, arg, name);
}

void *
mpic_intr_establish(int irqno, int level, int (*func)(void *),
    void *arg, char *name)
{
	struct mpic_softc	*sc = mpic;
	struct intrhand		*ih;
	int			 psw;

	if (irqno < 0 || irqno >= sc->sc_nintr)
		panic("%s: bogus irqnumber %d: %s", __func__,
		     irqno, name);

	/* no point in sleeping unless someone can free memory. */
	ih = (struct intrhand *)malloc (sizeof *ih, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("%s: can't malloc handler info", __func__);
	ih->ih_fun = func;
	ih->ih_arg = arg;
	ih->ih_ipl = level;
	ih->ih_irq = irqno;
	ih->ih_name = name;

	psw = intr_disable();

	TAILQ_INSERT_TAIL(&sc->sc_mpic_handler[irqno].is_list, ih, ih_list);

	if (name != NULL)
		evcount_attach(&ih->ih_count, name, &ih->ih_irq);

#ifdef DEBUG_INTC
	printf("%s irq %d level %d [%s]\n", __func__, irqno, level, name);
#endif
	mpic_calc_mask();
	
	intr_restore(psw);
	return (ih);
}

void
mpic_intr_disestablish(void *cookie)
{
#if 0
	intr_state_t its;
	struct intrhand *ih = cookie;
	int irqno = ih->ih_irq;
	its = intr_disable();
	TAILQ_REMOVE(&sc->sc_mpic_handler[irqno].is_list, ih, ih_list);
	if (ih->ih_name != NULL)
		evcount_detach(&ih->ih_count);
	free(ih, M_DEVBUF, 0);
	intr_restore(its);
#endif
}

const char *
mpic_intr_string(void *cookie)
{
	struct intrhand *ih = (struct intrhand *)cookie;
	static char irqstr[1 + sizeof("mpic irq ") + 4];

	snprintf(irqstr, sizeof irqstr, "mpic irq %d", ih->ih_irq);
	return irqstr;
}
