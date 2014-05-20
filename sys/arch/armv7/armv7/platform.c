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

#include <machine/bus.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/armv7/armv7_machdep.h>
#include <arm/cortex/smc.h>

#include "exynos.h"
#include "imx.h"
#include "omap.h"
#include "sunxi.h"

/* FIXME: Do not use a global variable. */
int isomap = 0;
static struct armv7_platform *platform;

void
platform_init()
{
	extern struct armv7_platform exynos_platform;
	extern struct armv7_platform imx_platform;
	extern struct armv7_platform omap_platform;
	extern struct armv7_platform sunxi_platform;

	switch (board_id) {
#if NEXYNOS > 0
	case BOARD_ID_EXYNOS5_CHROMEBOOK:
		platform = &exynos_platform;
		break;
#endif
#if NIMX > 0
	case BOARD_ID_IMX6_CUBOXI:
	case BOARD_ID_IMX6_HUMMINGBOARD:
	case BOARD_ID_IMX6_SABRELITE:
	case BOARD_ID_IMX6_UDOO:
	case BOARD_ID_IMX6_UTILITE:
	case BOARD_ID_IMX6_WANDBOARD:
		platform = &imx_platform;
		break;
#endif
#if NOMAP > 0
	case BOARD_ID_OMAP3_BEAGLE:
	case BOARD_ID_AM335X_BEAGLEBONE:
	case BOARD_ID_OMAP3_OVERO:
	case BOARD_ID_OMAP4_PANDA:
		platform = &omap_platform;
		isomap = 1;
		break;
#endif
#if NSUNXI > 0
	case BOARD_ID_SUN4I_A10:
	case BOARD_ID_SUN7I_A20:
		platform = &sunxi_platform;
		break;
#endif
	default:
		panic("%s: board type 0x%x unknown", __func__, board_id);
	}
}

const char *
platform_boot_name(void)
{
	return platform->boot_name;
}

void
platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	platform->smc_write(iot, ioh, off, op, val);
}

void
platform_init_cons(void)
{
	platform->init_cons();
}

void
platform_watchdog_reset(void)
{
	platform->watchdog_reset();
}

void
platform_powerdown(void)
{
	platform->powerdown();
}

void
platform_print_board_type(void)
{
	platform->print_board_type();
}

void
platform_disable_l2_if_needed(void)
{
	platform->disable_l2_if_needed();
}
