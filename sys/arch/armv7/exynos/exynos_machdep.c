/*	$OpenBSD	*/
/*
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
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/bootconfig.h>
#include <machine/fdt.h>

#include <arm/cortex/smc.h>
#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/exynos/exdisplayvar.h>
#include <armv7/armv7/armv7_machdep.h>

extern void exdog_reset(void);
extern int32_t agtimer_frequency;
extern int comcnspeed;
extern int comcnmode;

const char *platform_boot_name = "Bitrig/exynos";

void
platform_smc_write(bus_space_tag_t iot, bus_space_handle_t ioh, bus_size_t off,
    uint32_t op, uint32_t val)
{
	bus_space_write_4(iot, ioh, off, val);
}

void
platform_init_cons(void)
{
	paddr_t paddr;
	size_t size;
	void *node;

	switch (board_id) {
	case BOARD_ID_EXYNOS5_CHROMEBOOK:
		node = fdt_find_node("/framebuffer");
		if (node != NULL) {
			uint32_t *mem;
			if (fdt_node_property(node, "reg", (char **)&mem) >= 2*sizeof(uint32_t)) {
				paddr = betoh32(*mem++);
				size = betoh32(*mem);
			}
		}
		exdisplay_cnattach(&armv7_bs_tag, paddr, size);
		break;
	default:
		printf("board type %x unknown", board_id);
		return;
		/* XXX - HELP */
	}
}

void
platform_watchdog_reset(void)
{
	exdog_reset();
}

void
platform_powerdown(void)
{

}

void
platform_print_board_type(void)
{
	switch (board_id) {
	case BOARD_ID_EXYNOS5_CHROMEBOOK:
		agtimer_frequency = 24 * 1000 * 1000;
		printf("board type: Exynos 5 Chromebook\n");
		break;
	default:
		printf("board type %x unknown\n", board_id);
	}
}

void
platform_bootconfig_dram(BootConfig *bootconfig, psize_t *memstart, psize_t *memsize)
{
	if (bootconfig->dramblocks == 0) {
		*memstart = SDRAM_START;
		*memsize = 0x10000000; /* 256 MB */
		/* Fake bootconfig structure for the benefit of pmap.c */
		/* XXX must make the memory description h/w independant */
		bootconfig->dram[0].address = *memstart;
		bootconfig->dram[0].pages = *memsize / PAGE_SIZE;
		bootconfig->dramblocks = 1;
	} else {
		*memstart = bootconfig->dram[0].address;
		*memsize = bootconfig->dram[0].pages * PAGE_SIZE;
	}
}

void
platform_disable_l2_if_needed(void)
{

}
