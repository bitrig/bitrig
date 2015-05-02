/*
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <machine/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>

int	fdtcom_match(struct device *, void *, void *);
void	fdtcom_attach(struct device *, struct device *, void *);
int	fdtcom_dw_intr(void *);

struct cfattach com_fdt_ca = {
	sizeof (struct com_softc), fdtcom_match, fdtcom_attach,
};

int
fdtcom_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("snps,dw-apb-uart", aa->aa_node))
		return (1);

	return 0;
}

void
fdtcom_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_softc *sc = (struct com_softc *)self;
	struct armv7_attach_args *aa = aux;
	struct fdt_memory mem;
	struct clk *clk;

	clk = clk_fdt_get(aa->aa_node, 0);
	if (clk == NULL) {
		printf("%s: cannot get clock\n", sc->sc_dev.dv_xname);
		return;
	}

	clk_enable(clk);

	sc->sc_iot = &armv7_a4x_bs_tag;
	if (fdt_get_memory_address(aa->aa_node, 0, &mem)) {
		printf("%s: cannot extract memory\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iobase = mem.addr;
	sc->sc_frequency = clk_get_rate(clk) * 1000;
	sc->sc_uarttype = COM_UART_16550;

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh)) {
		printf("%s: bus_space_map failed\n", sc->sc_dev.dv_xname);
		return;
	}

	if (fdt_node_compatible("snps,dw-apb-uart", aa->aa_node))
		sc->sc_hwflags |= COM_HW_IIR_BUSY;

	com_attach_subr(sc);

	arm_intr_establish_fdt(aa->aa_node, IPL_TTY, comintr, sc,
	    sc->sc_dev.dv_xname);
}
