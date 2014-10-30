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
#include <machine/fdt.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <arm/cortex/smc.h>
#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/armv7/armv7_machdep.h>

#include <armv7/virt/pl011var.h>
#include <armv7/imx/imxuartvar.h>
#include <armv7/sunxi/sxiuartvar.h>
#include <armv7/exynos/exdisplayvar.h>

extern int comcnspeed;
extern int comcnmode;

static void
fdt_platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	bus_space_write_4(iot, ioh, off, val);
}

static void
fdt_platform_init_cons(void)
{
	void *node;
	char *stdout_path;
	struct fdt_memory mem;

	if ((node = fdt_find_compatible("simple-framebuffer")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		exdisplay_cnattach(&armv7_bs_tag, mem.addr, mem.size);

	if ((node = fdt_find_compatible("arm,pl011")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		pl011cnattach(&armv7_bs_tag, mem.addr, comcnspeed, comcnmode);

	if ((node = fdt_find_compatible("snps,dw-apb-uart")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		sxiuartcnattach(&armv7_a4x_bs_tag, mem.addr, comcnspeed,
		    24000000, comcnmode);

	if ((node = fdt_find_compatible("fsl,ns16550")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		comcnattach(&armv7_bs_tag, mem.addr, comcnspeed, 150000000, comcnmode);

	if ((node = fdt_find_node("/chosen")) == NULL ||
	    !fdt_node_property(node, "stdout-path", &stdout_path))
		return;

	if ((node = fdt_find_node(stdout_path)) == NULL)
		return;

	if (fdt_node_compatible("fsl,imx6q-uart", node) &&
	    !fdt_get_memory_address(node, 0, &mem))
		imxuartcnattach(&armv7_bs_tag, mem.addr, comcnspeed, comcnmode);

	comdefaultrate = comcnspeed;
}

static void
fdt_platform_watchdog_reset(void)
{
}

static void
fdt_platform_powerdown(void)
{

}

static void
fdt_platform_print_board_type(void)
{
	void *node = fdt_next_node(0);
	char *compatible;

	if (!fdt_node_property(node, "compatible", &compatible))
		printf("board type %x unknown\n", board_id);
	else
		printf("board: %s compatible\n", compatible);
}

static void
fdt_platform_disable_l2_if_needed(void)
{

}

struct armv7_platform fdt_platform = {
	.boot_name = "Bitrig/fdt",
	.smc_write = fdt_platform_smc_write,
	.init_cons = fdt_platform_init_cons,
	.watchdog_reset = fdt_platform_watchdog_reset,
	.powerdown = fdt_platform_powerdown,
	.print_board_type = fdt_platform_print_board_type,
	.disable_l2_if_needed = fdt_platform_disable_l2_if_needed,
};
