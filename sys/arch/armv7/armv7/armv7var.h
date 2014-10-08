/* $OpenBSD: armv7var.h,v 1.1 2013/11/06 19:08:06 syl Exp $ */
/*
 * Copyright (c) 2005,2008 Dale Rahn <drahn@openbsd.com>
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

#ifndef __ARMV7VAR_H__
#define __ARMV7VAR_H__

#include <machine/bootconfig.h>

/* Boards device list */
struct board_dev {
	char	*name;
	int	unit;
};

/* Needed by omap, imx, sunxi */
struct armv7_softc {
	struct device sc_dv;

	struct board_dev *sc_board_devs;
};

/* Physical memory range for on-chip devices. */

struct armv7mem {
	bus_addr_t	addr;
	bus_size_t	size;
};

#define ARMV7_DEV_NMEM 6
#define ARMV7_DEV_NIRQ 4
#define ARMV7_DEV_NDMA 4

/* Descriptor for all on-chip devices. */
struct armv7_dev {
	char	*name;			/* driver name or made up name */
	int	unit;			/* driver instance number or -1 */
	struct	armv7mem mem[ARMV7_DEV_NMEM]; /* memory ranges */
	int	irq[ARMV7_DEV_NIRQ];	/* IRQ number(s) */
	int	dma[ARMV7_DEV_NDMA];	/* DMA chan number(s) */
};

/* Passed as third arg to attach functions. */
struct armv7_attach_args {
	struct armv7_dev	*aa_dev;
	bus_space_tag_t		aa_iot;
	bus_dma_tag_t		aa_dmat;
	void			*aa_node;
};

extern struct armv7_dev *armv7_devs;

void	armv7_set_devs(struct armv7_dev *);
struct	armv7_dev *armv7_find_dev(const char *, int);
int	armv7_match(struct device *, void *, void *);
void	armv7_attach(struct device *, struct device *, void *);
int	armv7_submatch(struct device *, void *, void *);

/* board identification - from uboot */
#define BOARD_ID_OMAP3_BEAGLE 1546
#define BOARD_ID_OMAP3_OVERO 1798
#define BOARD_ID_OMAP4_PANDA 2791
#define BOARD_ID_AM335X_BEAGLEBONE 3589
#define BOARD_ID_IMX6_SABRELITE 3769
#define BOARD_ID_EXYNOS5_CHROMEBOOK 3774
#define BOARD_ID_SUN4I_A10 4104
#define BOARD_ID_IMX6_UTILITE 4273
#define BOARD_ID_SUN7I_A20 4283
#define BOARD_ID_IMX6_WANDBOARD 4412
#define BOARD_ID_IMX6_HUMMINGBOARD 4773
#define BOARD_ID_IMX6_UDOO 4800
#define BOARD_ID_IMX6_CUBOXI 4821
#define BOARD_ID_VIRT 0xffffffff
extern uint32_t board_id;

/* different arch init */
void am335x_init(void);
void exynos5_init(void);
void imx6_init(void);
void omap3_init(void);
void omap4_init(void);
void sxia1x_init(void);
void sxia20_init(void);

struct armv7_platform {
	const char *boot_name;
	void (*smc_write)(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint32_t, uint32_t);
	void (*init_cons)(void);
	void (*watchdog_reset)(void);
	void (*powerdown)(void);
	void (*print_board_type)(void);
	void (*bootconfig_dram)(BootConfig *, psize_t *, psize_t *);
	void (*disable_l2_if_needed)(void);
};

#endif /* __ARMV7VAR_H__ */

