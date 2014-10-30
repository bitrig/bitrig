/* $OpenBSD: omap_com.c,v 1.1 2013/09/04 14:38:31 patrick Exp $ */
/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/fdt.h>

#ifdef DDB
#include <ddb/db_var.h>
#endif

#define	COM_CONSOLE
#include <dev/cons.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <armv7/armv7/armv7var.h>

int	fsluart_match(struct device *, void *, void *);
void	fsluart_attach(struct device *, struct device *, void *);
int	fsluart_activate(struct device *, int);
int	fsluart_intr(void *);

struct fsluart_softc {
	struct device		sc_dev;
	struct com_softc	*sc;
	int			lsr;
};

struct cfdriver fsluart_cd = {
	NULL, "fsluart", DV_DULL
};

struct cfattach fsluart_ca = {
	sizeof (struct fsluart_softc), fsluart_match, fsluart_attach, NULL,
	fsluart_activate
};

int
fsluart_match(struct device *parent, void *self, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("fsl,ns16550", aa->aa_node))
		return 1;

	return 0;
}

void
fsluart_attach(struct device *parent, struct device *self, void *args)
{
	struct fsluart_softc *sc = (struct fsluart_softc *)self;
	struct armv7_attach_args *aa = args;
	struct fdt_memory mem;
	uint32_t freq = 0;

	if (aa->aa_node == NULL)
		panic("%s: no fdt node", __func__);

	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	if (!fdt_node_property_int(aa->aa_node, "clock-frequency", &freq))
		panic("%s: could not extract clock frequency from FDT", __func__);

	if (mem.addr == comconsaddr)
		printf(": console");

	printf("\n");

	if ((sc->sc = (struct com_softc *)config_found(self, NULL, NULL)) == NULL)
		goto out;

	/* TODO: Use 64-byte FIFO. */
	sc->sc->sc_iot = aa->aa_iot;
	sc->sc->sc_iobase = mem.addr;
	sc->sc->sc_frequency = freq;
	sc->sc->sc_uarttype = COM_UART_16550;

	if (bus_space_map(sc->sc->sc_iot, mem.addr, mem.size, 0, &sc->sc->sc_ioh)) {
		printf("%s: bus_space_map failed\n", __func__);
		return;
	}

	com_attach_subr(sc->sc);

	arm_intr_establish_fdt(aa->aa_node, IPL_TTY, fsluart_intr, sc,
	    sc->sc->sc_dev.dv_xname);

out:
	return;
}

int
fsluart_activate(struct device *self, int act)
{
	return 0;
}

/*
 * Same as comintr(), but fixes errata in fsl,ns16550.
 */
int
fsluart_intr(void *arg)
{
	struct fsluart_softc *fsl = arg;
	struct com_softc *sc = fsl->sc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct tty *tp;
	u_char lsr, data, msr, delta;

	if (ISSET(bus_space_read_1(iot, ioh, com_iir), IIR_NOPEND))
		return (0);

	if (ISSET(fsl->lsr, LSR_BI)) {
		fsl->lsr &= ~LSR_BI;
		bus_space_read_1(iot, ioh, com_data);
		return (1);
	}

	if (!sc->sc_tty) {
		bus_space_write_1(iot, ioh, com_ier, 0);
		return (0);		/* Can't do squat. */
	}
	tp = sc->sc_tty;

	for (;;) {
		fsl->lsr = lsr = bus_space_read_1(iot, ioh, com_lsr);

		if (ISSET(lsr, LSR_RXRDY)) {
			u_char *p = sc->sc_ibufp;

			softintr_schedule(sc->sc_si);
			do {
				data = bus_space_read_1(iot, ioh, com_data);
				if (ISSET(lsr, LSR_BI)) {
#if defined(COM_CONSOLE) && defined(DDB)
					if (ISSET(sc->sc_hwflags,
					    COM_HW_CONSOLE)) {
						if (db_console)
							Debugger();
						goto next;
					}
#endif
					data = 0;
				}
				if (p >= sc->sc_ibufend) {
					sc->sc_floods++;
					if (sc->sc_errors++ == 0)
						timeout_add_sec(&sc->sc_diag_tmo, 60);
				} else {
					*p++ = data;
					*p++ = lsr;
					if (p == sc->sc_ibufhigh &&
					    ISSET(tp->t_cflag, CRTSCTS)) {
						/* XXX */
						CLR(sc->sc_mcr, MCR_RTS);
						bus_space_write_1(iot, ioh, com_mcr,
						    sc->sc_mcr);
					}
				}
#if defined(COM_CONSOLE) && defined(DDB)
			next:
#endif
				fsl->lsr = lsr = bus_space_read_1(iot, ioh, com_lsr);
			} while (ISSET(lsr, LSR_RXRDY));

			sc->sc_ibufp = p;
		}
		msr = bus_space_read_1(iot, ioh, com_msr);

		if (msr != sc->sc_msr) {
			delta = msr ^ sc->sc_msr;

			ttytstamp(tp, sc->sc_msr & MSR_CTS, msr & MSR_CTS,
			    sc->sc_msr & MSR_DCD, msr & MSR_DCD);

			sc->sc_msr = msr;
			if (ISSET(delta, MSR_DCD)) {
				if (!ISSET(sc->sc_swflags, COM_SW_SOFTCAR) &&
				    (*linesw[tp->t_line].l_modem)(tp, ISSET(msr, MSR_DCD)) == 0) {
					CLR(sc->sc_mcr, sc->sc_dtr);
					bus_space_write_1(iot, ioh, com_mcr, sc->sc_mcr);
				}
			}
			if (ISSET(delta & msr, MSR_CTS) &&
			    ISSET(tp->t_cflag, CRTSCTS)) {
				/* the line is up and we want to do rts/cts flow control */
				(*linesw[tp->t_line].l_start)(tp);
			}
		}

		if (ISSET(lsr, LSR_TXRDY) && ISSET(tp->t_state, TS_BUSY)) {
			CLR(tp->t_state, TS_BUSY | TS_FLUSH);
			if (sc->sc_halt > 0)
				wakeup(&tp->t_outq);
			(*linesw[tp->t_line].l_start)(tp);
		}

		if (ISSET(bus_space_read_1(iot, ioh, com_iir), IIR_NOPEND))
			return (1);
	}
}

int	com_fsl_match(struct device *, void *, void *);
void	com_fsl_attach(struct device *, struct device *, void *);

struct cfattach com_fsl_ca = {
	sizeof (struct com_softc), com_fsl_match, com_fsl_attach
};

int
com_fsl_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
com_fsl_attach(struct device *parent, struct device *self, void *aux)
{
}
