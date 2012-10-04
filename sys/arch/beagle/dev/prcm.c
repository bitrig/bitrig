/* $OpenBSD: prcm.c,v 1.9 2011/11/10 19:37:01 uwe Exp $ */
/*
 * Copyright (c) 2007, 2009, 2012 Dale Rahn <drahn@dalerahn.com>
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

/*
 * Driver for the Power, Reset and Clock Management Module (PRCM).
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>
#include <beagle/dev/omapvar.h>
#include <beagle/dev/prcmvar.h>

#include <beagle/dev/omap3_prcmreg.h>
#include <beagle/dev/omap4_prcmreg.h>
/* XXX - some register defines are duplicated between omap3/omap4 need fixed */

#define LMAX(x, y) (((x) > (y)) ? (x) : (y))
#define PRCM_REG_MAX LMAX(O3_PRCM_REG_MAX, O4_PRCM_REG_MAX)
uint32_t prcm_imask_cur[PRCM_REG_MAX];
uint32_t prcm_fmask_cur[PRCM_REG_MAX];
uint32_t prcm_imask_mask[PRCM_REG_MAX];
uint32_t prcm_fmask_mask[PRCM_REG_MAX];
uint32_t prcm_imask_addr[PRCM_REG_MAX];
uint32_t prcm_fmask_addr[PRCM_REG_MAX];

#define SYS_CLK		13 /* SYS_CLK speed in MHz */

bus_space_tag_t		prcm_iot;
bus_space_handle_t	prcm_ioh;

bus_space_handle_t      cm1_ioh;
bus_space_handle_t      cm2_ioh;
bus_space_handle_t      scrm_ioh;
bus_space_handle_t      pcnf1_ioh;
bus_space_handle_t      pcnf2_ioh;

int     prcm_match(struct device *, void *, void *);
void    prcm_attach(struct device *, struct device *, void *);

int scrm_avail;
int pcnf1_avail;
int pcnf2_avail;

struct cfattach	prcm_ca = {
	sizeof (struct device), NULL, prcm_attach
};

struct cfdriver prcm_cd = {
	NULL, "prcm", DV_DULL
};

void
prcm_attach(struct device *parent, struct device *self, void *args)
{
	struct omap_attach_args *oa = args;

        switch (board_id) {
        case BOARD_ID_OMAP3_BEAGLE:
        case BOARD_ID_OMAP3_OVERO:
	    prcm_fmask_mask[PRCM_REG_CORE_CLK1] = O3_PRCM_REG_CORE_CLK1_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK1] = O3_PRCM_REG_CORE_CLK1_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK1] = O3_PRCM_REG_CORE_CLK1_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK1] = O3_PRCM_REG_CORE_CLK1_IADDR;

	    prcm_fmask_mask[PRCM_REG_CORE_CLK2] = O3_PRCM_REG_CORE_CLK2_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK2] = O3_PRCM_REG_CORE_CLK2_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK2] = O3_PRCM_REG_CORE_CLK2_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK2] = O3_PRCM_REG_CORE_CLK2_IADDR;

	    prcm_fmask_mask[PRCM_REG_CORE_CLK3] = O3_PRCM_REG_CORE_CLK3_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK3] = O3_PRCM_REG_CORE_CLK3_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK3] = O3_PRCM_REG_CORE_CLK3_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK3] = O3_PRCM_REG_CORE_CLK3_IADDR;

	    prcm_fmask_mask[PRCM_REG_USBHOST] = O3_PRCM_REG_USBHOST_FMASK;
	    prcm_imask_mask[PRCM_REG_USBHOST] = O3_PRCM_REG_USBHOST_IMASK;
	    prcm_fmask_addr[PRCM_REG_USBHOST] = O3_PRCM_REG_USBHOST_FADDR;
	    prcm_imask_addr[PRCM_REG_USBHOST] = O3_PRCM_REG_USBHOST_IADDR;
            break; 
        case BOARD_ID_OMAP4_PANDA:
	    prcm_fmask_mask[PRCM_REG_CORE_CLK1] = O4_PRCM_REG_CORE_CLK1_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK1] = O4_PRCM_REG_CORE_CLK1_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK1] = O4_PRCM_REG_CORE_CLK1_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK1] = O4_PRCM_REG_CORE_CLK1_IADDR;

	    prcm_fmask_mask[PRCM_REG_CORE_CLK2] = O4_PRCM_REG_CORE_CLK2_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK2] = O4_PRCM_REG_CORE_CLK2_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK2] = O4_PRCM_REG_CORE_CLK2_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK2] = O4_PRCM_REG_CORE_CLK2_IADDR;

	    prcm_fmask_mask[PRCM_REG_CORE_CLK3] = O4_PRCM_REG_CORE_CLK3_FMASK;
	    prcm_imask_mask[PRCM_REG_CORE_CLK3] = O4_PRCM_REG_CORE_CLK3_IMASK;
	    prcm_fmask_addr[PRCM_REG_CORE_CLK3] = O4_PRCM_REG_CORE_CLK3_FADDR;
	    prcm_imask_addr[PRCM_REG_CORE_CLK3] = O4_PRCM_REG_CORE_CLK3_IADDR;

	    prcm_fmask_mask[PRCM_REG_USBHOST] = O4_PRCM_REG_USBHOST_FMASK;
	    prcm_imask_mask[PRCM_REG_USBHOST] = O4_PRCM_REG_USBHOST_IMASK;
	    prcm_fmask_addr[PRCM_REG_USBHOST] = O4_PRCM_REG_USBHOST_FADDR;
	    prcm_imask_addr[PRCM_REG_USBHOST] = O4_PRCM_REG_USBHOST_IADDR;
	    scrm_avail = 1;
	    pcnf1_avail = 1;
	    pcnf2_avail = 1;
	}
	prcm_iot = oa->oa_iot;

	if (bus_space_map(prcm_iot, oa->oa_dev->mem[0].addr,
	    oa->oa_dev->mem[0].size, 0, &prcm_ioh))
		panic("prcm_attach: bus_space_map failed!");

	if (bus_space_map(prcm_iot, oa->oa_dev->mem[1].addr,
	    oa->oa_dev->mem[1].size, 0, &cm1_ioh))
		panic("prcm_attach: bus_space_map failed!");

	if (bus_space_map(prcm_iot, oa->oa_dev->mem[2].addr,
	    oa->oa_dev->mem[2].size, 0, &cm2_ioh))
		panic("prcm_attach: bus_space_map failed!");

	if (scrm_avail) {
		if (bus_space_map(prcm_iot, oa->oa_dev->mem[3].addr,
		    oa->oa_dev->mem[3].size, 0, &scrm_ioh))
			panic("prcm_attach: bus_space_map failed!");
	}
	if (pcnf1_avail) {
		if (bus_space_map(prcm_iot, oa->oa_dev->mem[4].addr,
		    oa->oa_dev->mem[4].size, 0, &pcnf1_ioh))
			panic("prcm_attach: bus_space_map failed!");
	}
	if (pcnf2_avail) {
		if (bus_space_map(prcm_iot, oa->oa_dev->mem[5].addr,
		    oa->oa_dev->mem[5].size, 0, &pcnf2_ioh))
			panic("prcm_attach: bus_space_map failed!");
	}

	printf("\n");
	

	/* XXX */
#if 1
	printf("CM_FCLKEN1_CORE %x\n", bus_space_read_4(prcm_iot, prcm_ioh, CM_FCLKEN1_CORE));
	printf("CM_ICLKEN1_CORE %x\n", bus_space_read_4(prcm_iot, prcm_ioh, CM_ICLKEN1_CORE));
	printf("CM_AUTOIDLE1_CORE %x\n", bus_space_read_4(prcm_iot, prcm_ioh, CM_AUTOIDLE1_CORE));

	printf("CM_FCLKEN_WKUP %x\n", bus_space_read_4(prcm_iot, prcm_ioh, CM_FCLKEN_WKUP));
	printf(" CM_IDLEST_WKUP %x\n", bus_space_read_4(prcm_iot, prcm_ioh,  CM_IDLEST_WKUP));
//	bus_space_write_4(prcm_iot, prcm_ioh, 
#endif

#if 0
	reg = bus_space_read_4(prcm_iot, prcm_ioh, CM_FCLKEN1_CORE);
	reg |= CM_FCLKEN1_CORE_GP3|CM_FCLKEN1_CORE_GP2;
	bus_space_write_4(prcm_iot, prcm_ioh, CM_FCLKEN1_CORE, reg);
	reg = bus_space_read_4(prcm_iot, prcm_ioh, CM_ICLKEN1_CORE);
	reg |= CM_ICLKEN1_CORE_GP3|CM_ICLKEN1_CORE_GP2;
	bus_space_write_4(prcm_iot, prcm_ioh, CM_ICLKEN1_CORE, reg);

	reg = bus_space_read_4(prcm_iot, prcm_ioh, CM_FCLKEN_WKUP);
	reg |= CM_FCLKEN_WKUP_MPU_WDT | CM_FCLKEN_WKUP_GPT1;
	bus_space_write_4(prcm_iot, prcm_ioh, CM_FCLKEN_WKUP, reg);

	reg = bus_space_read_4(prcm_iot, prcm_ioh, CM_ICLKEN_WKUP);
	reg |= CM_ICLKEN_WKUP_MPU_WDT | CM_ICLKEN_WKUP_GPT1;
	bus_space_write_4(prcm_iot, prcm_ioh, CM_ICLKEN_WKUP, reg);
#endif

}

void
prcm_setclock(int clock, int speed)
{
#if 1
	u_int32_t oreg, reg, mask;
	if (clock == 1) {
		oreg = bus_space_read_4(prcm_iot, prcm_ioh, CM_CLKSEL_WKUP);
		mask = 1;
		reg = (oreg &~mask) | (speed & mask);
		printf(" prcm_setclock old %08x new %08x",  oreg, reg );
		bus_space_write_4(prcm_iot, prcm_ioh, CM_CLKSEL_WKUP, reg);
	} else if (clock >= 2 && clock <= 9) {
		int shift =  (clock-2);
		oreg = bus_space_read_4(prcm_iot, prcm_ioh, CM_CLKSEL_PER);

		mask = 1 << (shift);
		reg =  (oreg & ~mask) | ( (speed << shift) & mask);
		printf(" prcm_setclock old %08x new %08x",  oreg, reg);

		bus_space_write_4(prcm_iot, prcm_ioh, CM_CLKSEL_PER, reg);
	} else
		panic("prcm_setclock invalid clock %d", clock);
#endif
}

void
prcm_enableclock(int bit)
{
	u_int32_t fclk, iclk, fmask, imask, mbit;
	int freg, ireg, reg;
	printf("prcm_enableclock %d:", bit);

	reg = bit >> 5;

	freg = prcm_fmask_addr[reg];
	ireg = prcm_imask_addr[reg];
	fmask = prcm_fmask_mask[reg];
	imask = prcm_imask_mask[reg];

	mbit =  1 << (bit & 0x1f);
#if 0
	printf("reg %d faddr 0x%08x iaddr 0x%08x, fmask 0x%08x "
	    "imask 0x%08x mbit %x", reg, freg, ireg, fmask, imask, mbit);
#endif
	if (fmask & mbit) { /* dont access the register if bit isn't present */
		fclk = bus_space_read_4(prcm_iot, prcm_ioh, freg);
		prcm_fmask_cur[reg] = fclk | mbit;
		bus_space_write_4(prcm_iot, prcm_ioh, freg, fclk | mbit);
		printf(" fclk %08x %08x",  fclk, fclk | mbit);
	}

	if (imask & mbit) { /* dont access the register if bit isn't present */
		iclk = bus_space_read_4(prcm_iot, prcm_ioh, ireg);
		prcm_imask_cur[reg] = iclk | mbit;
		bus_space_write_4(prcm_iot, prcm_ioh, ireg, iclk | mbit);
		printf(" iclk %08x %08x",  iclk, iclk | mbit);
	}
	printf ("\n");
}

uint32_t
prcm_scrm_get_value(uint32_t reg)
{
	uint32_t val;
	// fix
        val = bus_space_read_4(prcm_iot, scrm_ioh, reg);
        return val;
}

void
prcm_scrm_set_value(uint32_t reg, uint32_t val)
{
	bus_space_write_4(prcm_iot, scrm_ioh, reg, val);
}

int prcm_hsusbhost_activate(int type);
int
prcm_hsusbhost_activate(int type)
{
	return prcm_hsusbhost_activate_omap4(type);
}
int
prcm_hsusbhost_deactivate(int type)
{
	return prcm_hsusbhost_deactivate_omap4(type);
}

int
prcm_hsusbhost_deactivate_omap4(int type)
{
	uint32_t clksel_reg_off;
	uint32_t clksel;
	bus_space_tag_t		*iot;
	bus_space_handle_t	*ioh;

	switch (type) {
#if 0
	/* XXX - unused ? */
		case USBTLL_CLK:
			/* We need the CM_L3INIT_HSUSBTLL_CLKCTRL register in CM2 register set */
			iot = &prcm_iot;
			ioh = &cm2_ioh;
			clksel_reg_off = L3INIT_CM2_OFFSET + 0x68;

			clksel = bus_space_read_4(*iot, *ioh, clksel_reg_off);
			clksel &= ~CLKCTRL_MODULEMODE_MASK;
			clksel |=  CLKCTRL_MODULEMODE_DISABLE;
			break;

#endif
		case USBHSHOST_CLK:
		case USBP1_PHY_CLK:
		case USBP2_PHY_CLK:
#if 0
		case USBP1_UTMI_CLK:
		case USBP2_UTMI_CLK:
		case USBP3_UTMI_CLK:
		case USBP1_HSIC_CLK:
		case USBP2_HSIC_CLK:
#endif
			/* For the USB HS HOST module we need to enable the following clocks:
			 *  - INIT_L4_ICLK     (will be enabled by bootloader)
			 *  - INIT_L3_ICLK     (will be enabled by bootloader)
			 *  - INIT_48MC_FCLK
			 *  - UTMI_ROOT_GFCLK  (UTMI only, create a new clock for that ?)
			 *  - UTMI_P1_FCLK     (UTMI only, create a new clock for that ?)
			 *  - UTMI_P2_FCLK     (UTMI only, create a new clock for that ?)
			 *  - UTMI_P3_FCLK     (UTMI only, create a new clock for that ?)
			 *  - HSIC_P1_60       (HSIC only, create a new clock for that ?)
			 *  - HSIC_P1_480      (HSIC only, create a new clock for that ?)
			 *  - HSIC_P2_60       (HSIC only, create a new clock for that ?)
			 *  - HSIC_P2_480      (HSIC only, create a new clock for that ?)
			 */

			/* We need the CM_L3INIT_HSUSBHOST_CLKCTRL register in CM2 register set */
			iot = &prcm_iot;
			ioh = &cm2_ioh;
			clksel_reg_off = O4_L3INIT_CM2_OFFSET + 0x58;
			clksel = bus_space_read_4(*iot, *ioh, clksel_reg_off);

			/* Enable the module and also enable the optional func clocks */
			if (type == USBHSHOST_CLK) {
				clksel &= ~O4_CLKCTRL_MODULEMODE_MASK;
				clksel |=  O4_CLKCTRL_MODULEMODE_DISABLE;

				clksel &= ~(0x1 << 15); /* USB-HOST clock control: FUNC48MCLK */
			}

#if 0
			else if (type == USBP1_UTMI_CLK)
				clksel &= ~(0x1 << 8);  /* UTMI_P1_CLK */
			else if (type == USBP2_UTMI_CLK)
				clksel &= ~(0x1 << 9);  /* UTMI_P2_CLK */
			else if (type == USBP3_UTMI_CLK)
				clksel &= ~(0x1 << 10);  /* UTMI_P3_CLK */

			else if (type == USBP1_HSIC_CLK)
				clksel &= ~(0x5 << 11);  /* HSIC60M_P1_CLK + HSIC480M_P1_CLK */
			else if (type == USBP2_HSIC_CLK)
				clksel &= ~(0x5 << 12);  /* HSIC60M_P2_CLK + HSIC480M_P2_CLK */
#endif
			else {
				panic("%s:missing type %d\n", type);
			}

			break;

		default:
			panic("%s: invalid type %d", type);
			return (EINVAL);
	}

	bus_space_write_4(*iot, *ioh, clksel_reg_off, clksel);

	return (0);
}

int
prcm_hsusbhost_activate_omap4(int type)
{
	uint32_t i;
	uint32_t clksel_reg_off;
	uint32_t clksel, oclksel;
	bus_space_tag_t		*iot;
	bus_space_handle_t	*ioh;
	switch (type) {
#if 0
		case USBTLL_CLK:
			/* For the USBTLL module we need to enable the following clocks:
			 *  - INIT_L4_ICLK  (will be enabled by bootloader)
			 *  - TLL_CH0_FCLK
			 *  - TLL_CH1_FCLK
			 */

			/* We need the CM_L3INIT_HSUSBTLL_CLKCTRL register in CM2 register set */
			iot = &prcm_iot;
			ioh = &cm2_ioh;
			clksel_reg_off = L3INIT_CM2_OFFSET + 0x68;

			/* Enable the module and also enable the optional func clocks for
			 * channels 0 & 1 (is this needed ?)
			 */
			clksel = bus_space_read_4(*iot, *ioh, clksel_reg_off);
			oclksel = clksel;
			clksel &= ~CLKCTRL_MODULEMODE_MASK;
			clksel |=  CLKCTRL_MODULEMODE_ENABLE;

			clksel |= (0x1 << 8); /* USB-HOST optional clock: USB_CH0_CLK */
			clksel |= (0x1 << 9); /* USB-HOST optional clock: USB_CH1_CLK */
			break;
#endif
		case USBHSHOST_CLK:
		case USBP1_PHY_CLK:
		case USBP2_PHY_CLK:
#if 0
		case USBP1_UTMI_CLK:
		case USBP2_UTMI_CLK:
		case USBP3_UTMI_CLK:
		case USBP1_HSIC_CLK:
		case USBP2_HSIC_CLK:
#endif
			/* For the USB HS HOST module we need to enable the following clocks:
			 *  - INIT_L4_ICLK     (will be enabled by bootloader)
			 *  - INIT_L3_ICLK     (will be enabled by bootloader)
			 *  - INIT_48MC_FCLK
			 *  - UTMI_ROOT_GFCLK  (UTMI only, create a new clock for that ?)
			 *  - UTMI_P1_FCLK     (UTMI only, create a new clock for that ?)
			 *  - UTMI_P2_FCLK     (UTMI only, create a new clock for that ?)
			 *  - UTMI_P3_FCLK     (UTMI only, create a new clock for that ?)
			 *  - HSIC_P1_60       (HSIC only, create a new clock for that ?)
			 *  - HSIC_P1_480      (HSIC only, create a new clock for that ?)
			 *  - HSIC_P2_60       (HSIC only, create a new clock for that ?)
			 *  - HSIC_P2_480      (HSIC only, create a new clock for that ?)
			 */

			/* We need the CM_L3INIT_HSUSBHOST_CLKCTRL register in CM2 register set */
			iot = &prcm_iot;
			ioh = &cm2_ioh;
			clksel_reg_off = O4_L3INIT_CM2_OFFSET + 0x58;
			clksel = bus_space_read_4(*iot, *ioh, clksel_reg_off);
			oclksel = clksel;
			/* Enable the module and also enable the optional func clocks */
			if (type == USBHSHOST_CLK) {
				clksel &= ~O4_CLKCTRL_MODULEMODE_MASK;
				clksel |=  /*O4_CLKCTRL_MODULEMODE_ENABLE*/2;

				clksel |= (0x1 << 15); /* USB-HOST clock control: FUNC48MCLK */
			}

#if 0
			else if (type == USBP1_UTMI_CLK)
				clksel |= (0x1 << 8);  /* UTMI_P1_CLK */
			else if (type == USBP2_UTMI_CLK)
				clksel |= (0x1 << 9);  /* UTMI_P2_CLK */
			else if (type == USBP3_UTMI_CLK)
				clksel |= (0x1 << 10);  /* UTMI_P3_CLK */

			else if (type == USBP1_HSIC_CLK)
				clksel |= (0x5 << 11);  /* HSIC60M_P1_CLK + HSIC480M_P1_CLK */
			else if (type == USBP2_HSIC_CLK)
				clksel |= (0x5 << 12);  /* HSIC60M_P2_CLK + HSIC480M_P2_CLK */
#endif

			break;

		default:
			panic("%s: invalid type %d", type);
			return (EINVAL);
	}
	bus_space_write_4(*iot, *ioh, clksel_reg_off, clksel);

	/* Try MAX_MODULE_ENABLE_WAIT number of times to check if enabled */
	for (i = 0; i < O4_MAX_MODULE_ENABLE_WAIT; i++) {
		clksel = bus_space_read_4(*iot, *ioh, clksel_reg_off);
		if ((clksel & O4_CLKCTRL_IDLEST_MASK) == O4_CLKCTRL_IDLEST_ENABLED)
			break;
	}

	/* Check the enabled state */
	if ((clksel & O4_CLKCTRL_IDLEST_MASK) != O4_CLKCTRL_IDLEST_ENABLED) {
		printf("Error: HERE failed to enable module with clock %d\n", type);
		printf("Error: 0x%08x => 0x%08x\n", clksel_reg_off, clksel);
		return (ETIMEDOUT);
	}

	return (0);
}

int
prcm_hsusbhost_set_source(int clk, int clksrc)
{
//XXX omap4 version !?!?!?!
	uint32_t clksel_reg_off;
	uint32_t clksel;
	unsigned int bit;

	if (clk == USBP1_PHY_CLK)
		bit = 24;
	else if (clk != USBP2_PHY_CLK)
		bit = 25;
	else
		return (-EINVAL);

	/* We need the CM_L3INIT_HSUSBHOST_CLKCTRL register in CM2 register set */
	clksel_reg_off = O4_L3INIT_CM2_OFFSET + 0x58;
	clksel = bus_space_read_4(prcm_iot, cm2_ioh, clksel_reg_off);

	/* Set the clock source to either external or internal */
	if (clksrc == EXT_CLK)
		clksel |= (0x1 << bit);
	else
		clksel &= ~(0x1 << bit);

	bus_space_write_4(prcm_iot, cm2_ioh, clksel_reg_off, clksel);

	return (0);
}

uint32_t
prcm_pcnf1_get_value(uint32_t reg)
{
	uint32_t val;
        val =  bus_space_read_4(prcm_iot, scrm_ioh, reg);
        return val;
}
void
prcm_pcnf1_set_value(uint32_t reg, uint32_t val)
{
	bus_space_write_2(prcm_iot, pcnf1_ioh, reg, val);
}

uint32_t
prcm_pcnf2_get_value(uint32_t reg)
{
	uint32_t val;
	val = bus_space_read_4(prcm_iot, pcnf2_ioh, reg);
	return val;
}
void
prcm_pcnf2_set_value(uint32_t reg, uint32_t val)
{
	bus_space_write_4(prcm_iot, pcnf2_ioh, reg, val);
}
