/* $OpenBSD: exvar.h,v 1.1 2011/11/10 19:37:01 uwe Exp $ */
/*
 * Copyright (c) 2005,2008 Dale Rahn <drahn@drahn.com>
 * Copyright (c) 2012-2013 Patrick Wildt <patrick@blueri.se>
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

/* Physical memory range for on-chip devices. */
struct ex_mem {
	u_int32_t addr;			/* physical start address */
	u_int32_t size;			/* size of range in bytes */
};

#define EXYNOS_DEV_NMEM 6		/* number of memory ranges */
#define EXYNOS_DEV_NIRQ 4		/* number of IRQs per device */

/* Descriptor for all on-chip devices. */
struct ex_dev {
	char *name;			/* driver name or made up name */
	int unit;			/* driver instance number or -1 */
	struct ex_mem mem[EXYNOS_DEV_NMEM]; /* memory ranges */
	int irq[EXYNOS_DEV_NIRQ];	/* IRQ number(s) */
};

/* Passed as third arg to attach functions. */
struct ex_attach_args {
	struct ex_dev *ea_dev;
	bus_space_tag_t	ea_iot;
	bus_dma_tag_t ea_dmat;
};

void ex_set_devs(struct ex_dev *);
struct ex_dev *ex_find_dev(const char *, int);

void exynos5_init(void);

/* XXX */
void *avic_intr_establish(int irqno, int level, int (*func)(void *),
    void *arg, char *name);

/* board identification - from uboot */
#define BOARD_ID_EXYNOS5_CHROMEBOOK 3774
extern uint32_t board_id;
