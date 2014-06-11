/*	$OpenBSD	*/
/*
 * Copyright (c) 2013 Sylvestre Gallon <ccna.syl@gmail.com>
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

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/cortex/smc.h>
#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxuartvar.h>
#include <armv7/armv7/armv7_machdep.h>

extern void imxdog_reset(void);
extern int32_t amptimer_frequency;
extern int comcnspeed;
extern int comcnmode;

static void
imx_platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	bus_space_write_4(iot, ioh, off, val);
}

static void
imx_platform_init_cons(void)
{
	paddr_t paddr;

	switch (board_id) {
	case BOARD_ID_IMX6_HUMMINGBOARD:
		paddr = 0x02020000;
		break;
	case BOARD_ID_IMX6_SABRELITE:
		paddr = 0x021e8000;
		break;
	case BOARD_ID_IMX6_UDOO:
		paddr = 0x021e8000;
		break;
	case BOARD_ID_IMX6_UTILITE:
		paddr = 0x021f0000;
		break;
	case BOARD_ID_IMX6_WANDBOARD:
		paddr = 0x02020000;
		break;
	default:
		printf("board type %x unknown", board_id);
		return;
		/* XXX - HELP */
	}
	imxuartcnattach(&armv7_bs_tag, paddr, comcnspeed, comcnmode);
}

static void
imx_platform_watchdog_reset(void)
{
	imxdog_reset();
}

static void
imx_platform_powerdown(void)
{

}

static void
imx_platform_print_board_type(void)
{
	switch (board_id) {
	case BOARD_ID_IMX6_HUMMINGBOARD:
		amptimer_frequency = 396 * 1000 * 1000;
		printf("board type: HummingBoard\n");
		break;
	case BOARD_ID_IMX6_SABRELITE:
		amptimer_frequency = 396 * 1000 * 1000;
		printf("board type: SABRE Lite\n");
		break;
	case BOARD_ID_IMX6_UDOO:
		amptimer_frequency = 396 * 1000 * 1000;
		printf("board type: UDOO\n");
		break;
	case BOARD_ID_IMX6_UTILITE:
		amptimer_frequency = 396 * 1000 * 1000;
		printf("board type: Utilite\n");
		break;
	case BOARD_ID_IMX6_WANDBOARD:
		amptimer_frequency = 396 * 1000 * 1000;
		printf("board type: Wandboard\n");
		break;
	default:
		printf("board type %x unknown\n", board_id);
	}
}

static void
imx_platform_disable_l2_if_needed(void)
{

}

struct armv7_platform imx_platform = {
	.boot_name = "Bitrig/imx",
	.smc_write = imx_platform_smc_write,
	.init_cons = imx_platform_init_cons,
	.watchdog_reset = imx_platform_watchdog_reset,
	.powerdown = imx_platform_powerdown,
	.print_board_type = imx_platform_print_board_type,
	.disable_l2_if_needed = imx_platform_disable_l2_if_needed,
};
