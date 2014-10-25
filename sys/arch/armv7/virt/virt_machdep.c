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

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/cortex/smc.h>
#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/virt/pl011var.h>
#include <armv7/armv7/armv7_machdep.h>

extern int comcnspeed;
extern int comcnmode;

static void
virt_platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	bus_space_write_4(iot, ioh, off, val);
}

static void
virt_platform_init_cons(void)
{
	paddr_t paddr;

	switch (board_id) {
	case BOARD_ID_VIRT:
		paddr = 0x9000000;
		break;
	default:
		printf("board type %x unknown", board_id);
		return;
		/* XXX - HELP */
	}
	pl011cnattach(&armv7_bs_tag, paddr, comcnspeed, comcnmode);
}

static void
virt_platform_watchdog_reset(void)
{
}

static void
virt_platform_powerdown(void)
{

}

static void
virt_platform_print_board_type(void)
{
	switch (board_id) {
	case BOARD_ID_VIRT:
		printf("board type: Virt\n");
		break;
	default:
		printf("board type %x unknown\n", board_id);
	}
}

static void
virt_platform_disable_l2_if_needed(void)
{

}

struct armv7_platform virt_platform = {
	.boot_name = "Bitrig/virt",
	.smc_write = virt_platform_smc_write,
	.init_cons = virt_platform_init_cons,
	.watchdog_reset = virt_platform_watchdog_reset,
	.powerdown = virt_platform_powerdown,
	.print_board_type = virt_platform_print_board_type,
	.disable_l2_if_needed = virt_platform_disable_l2_if_needed,
};
