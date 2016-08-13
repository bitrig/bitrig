/* $Bitrig$ */
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
#include <machine/fdt.h>
#include <sys/evcount.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <arm64/arm64/arm64var.h>

int		cpu_fdt_match(struct device *, void *, void *);
void		cpuattach(struct device *, struct device *, void *);

struct cpu_softc {
	struct device		sc_dev;
};


struct cfattach cpu_fdt_ca = {
	sizeof (struct cpu_softc), cpu_fdt_match, cpuattach
};

struct cfdriver cpu_cd = {
	NULL, "cpu", DV_DULL
};


int
cpu_fdt_match(struct device *parent, void *cfdata, void *aux)
{
	struct arm64_attach_args *ca = aux;

	// XXX  arm64
	if (fdt_node_compatible("arm,cortex-a57", ca->aa_node))
		return (1);

	return 0;
}
