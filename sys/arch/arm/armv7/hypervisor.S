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

#include <arm/armreg.h>
#include <machine/asm.h>

#define ERET	.long	0xe160006e		@ eret
#define HVC	.long	0xe1400070		@ hvc #0

ENTRY_NP(hypervisor_get_vectors)
	mov	r0, #-1
	HVC
	mov	pc, lr

ENTRY_NP(hypervisor_set_vectors)
	HVC
	mov	pc, lr

ENTRY_NP(hypervisor_stub_install)
	/*
	 * Check that we are in hypervisor mode. Otherwise there's
	 * nothing to do for us here.
	 */
	mrs	r0, cpsr
	and	r0, r0, #(PSR_MODE)
	cmp	r0, #(PSR_HYP32_MODE)
	movne	pc, lr

	/*
	 * Install hypervisor stub vectors.
	 */
	adr	r0, hypervisor_stub_vectors
	mcr	p15, 4, r0, c12, c0, 0	@ set HVBAR

	/*
	 * Set up clean, initial values in hypervisor registers.
	 */
	mov	r0, #0
	mcr	p15, 4, r0, c1, c1, 0	@ HCR
	mcr	p15, 4, r0, c1, c1, 2	@ HCPTR
	mcr	p15, 4, r0, c1, c1, 3	@ HSTR
	mcr	p15, 4, r0, c1, c0, 0	@ HSCTLR

	mrc	p15, 4, r0, c1, c1, 1	@ HDCR
	and	r0, #0x1f		@ Preserve HPMN
	mcr	p15, 4, r0, c1, c1, 1	@ HDCR

#if 0
	/*
	 * Enable access to architected timers.
	 * First, make CNTP_* and CNTPCT accessible from PL1.
	 * XXX: Re-enable when we actually use this.
	 */
	mrc	p15, 0, r0, c0, c1, 1	@ ID_PFR1
	lsr	r0, #16
	and	r0, #0xf
	cmp	r0, #1
	bne	1f
	mrc	p15, 4, r0, c14, c1, 0	@ CNTHCTL
	orr	r0, r0, #3		@ PL1PCEN | PL1PCTEN
	mcr	p15, 4, r0, c14, c1, 0	@ CNTHCTL
	mov	r0, #0
	mcrr	p15, 4, r0, r0, c14	@ CNTVOFF
#endif

	/*
	 * Then, disable the virtual timer in case it was counting.
	 */
	mrc	p15, 0, r0, c14, c3, 1	@ CNTV_CTL
	bic	r0, #1			@ Clear ENABLE
	mcr	p15, 0, r0, c14, c3, 1	@ CNTV_CTL
1:

	mov	pc, lr

ENTRY_NP(hypervisor_stub_do_trap)
	cmp	r0, #-1
	mrceq	p15, 4, r0, c12, c0, 0	@ get HVBAR
	mcrne	p15, 4, r0, c12, c0, 0	@ set HVBAR
	ERET

.align 5
hypervisor_stub_vectors:
	b	.				@ reset
	b	.				@ undefined
	b	.				@ svc
	b	.				@ prefetch abort
	b	.				@ data abort
	b	hypervisor_stub_do_trap		@ trap
	b	.				@ irq
	b	.				@ fiq
