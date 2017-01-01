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

//#include <arm/cortex/smc.h>
//#include <arm/armv7/armv7var.h>
//#include <armv7/armv7/armv7_machdep.h>

#include <arm64/arm64/arm64var.h>
#include <arm64/virt/pl011var.h>
#include <arm64/dev/msmuartvar.h>
//#include <armv7/imx/imxuartvar.h>
//#include <armv7/sunxi/sxiuartvar.h>
//#include <armv7/exynos/exdisplayvar.h>

extern int comcnspeed;
extern int comcnmode;

#if 0
static void
fdt_platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	bus_space_write_4(iot, ioh, off, val);
}
#endif

void
fdt_platform_init_cons(void)
{
	void *node;
	// char *stdout_path;
	struct fdt_memory mem;
	uint32_t freq;

#if 0
	/*
	 * XXX: Without attaching the clocks we cannot find out
	 * XXX: the frequency we need. Thus try to make an educated
	 * XXX: guess about the frequency used.
	 */
	if (fdt_find_compatible("marvell,armada-370-xp"))
		freq = 250000000;
	else if (fdt_find_compatible("allwinner,sun7i-a20"))
		freq = 24000000;
	else
#endif
		freq = COM_FREQ;

#if 0
	if ((node = fdt_find_compatible("simple-framebuffer")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		exdisplay_cnattach(&arm64_bs_tag, mem.addr, mem.size);
#endif

	if ((node = fdt_find_compatible("arm,pl011")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		pl011cnattach(&arm64_bs_tag, mem.addr, comcnspeed, comcnmode);

	if ((node = fdt_find_compatible("snps,dw-apb-uart")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		comcnattach(&arm64_a4x_bs_tag, mem.addr, comcnspeed,
		    freq, comcnmode);

	if ((node = fdt_find_compatible("qcom,msm-uartdm-v1.4")) != NULL &&
	    !fdt_get_memory_address(node, 0, &mem))
		msmuartcnattach(&arm64_bs_tag, mem.addr, comcnspeed,
		    comcnmode);

if (node == 0) {
asm("mrs x0, scr_el3");
}

	if ((((node = fdt_find_compatible("fsl,ns16550")) != NULL) ||
	    ((node = fdt_find_compatible("fsl,16550-FIFO64")) != NULL)) &&
	    !fdt_get_memory_address(node, 0, &mem))
		comcnattach(&arm64_bs_tag, mem.addr, comcnspeed,
		    150000000, comcnmode);
#if 0
	if ((node = fdt_find_node("/chosen")) == NULL ||
	    !fdt_node_property(node, "stdout-path", &stdout_path))
		return;

	if ((node = fdt_find_node(stdout_path)) == NULL)
		return;

	if (fdt_node_compatible("fsl,imx6q-uart", node) &&
	    !fdt_get_memory_address(node, 0, &mem))
		imxuartcnattach(&armv7_bs_tag, mem.addr, comcnspeed, comcnmode);
#endif

	comdefaultrate = comcnspeed;
}
#if 0

void (*fdt_platform_watchdog_reset_fn)(void);
static void
fdt_platform_watchdog_reset(void)
{
	if (fdt_platform_watchdog_reset_fn != NULL)
		fdt_platform_watchdog_reset_fn();
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
		printf("board type %x unknown\n", 0x123);
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
#endif
