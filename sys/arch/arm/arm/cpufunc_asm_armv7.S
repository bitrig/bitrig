/*-
 * Copyright (c) 2010 Per Odlund <per.odlund@armagedon.se>
 * Copyright (C) 2011 MARVELL INTERNATIONAL LTD.
 * All rights reserved.
 *
 * Developed by Semihalf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of MARVELL nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arm/armreg.h>
#include <machine/asm.h>

.Lcoherency_level:
	.word	_C_LABEL(arm_cache_loc)
.Lcache_type:
	.word	_C_LABEL(arm_cache_type)
.Larmv7_dcache_line_size:
	.word	_C_LABEL(arm_dcache_min_line_size)
.Larmv7_icache_line_size:
	.word	_C_LABEL(arm_icache_min_line_size)
.Larmv7_idcache_line_size:
	.word	_C_LABEL(arm_idcache_min_line_size)
.Lway_mask:
	.word	0x3ff
.Lmax_index:
	.word	0x7fff
.Lpage_mask:
	.word	0xfff

ENTRY(armv7_cpu_sleep)
	dsb
	wfi
	mov	pc, lr

ENTRY(armv7_drain_writebuf)
	dsb
	mov	pc, lr

ENTRY(armv7_sev)
	dsb
	sev
	nop
	mov	pc, lr

/*
 * Function to read the MPCore base address
 */
ENTRY(armv7_periphbase)
	mrc	p15, 4, r0, c15, c0, 0
	mov	pc, lr

/*
 * Functions to set the MMU Translation Table Base register
 */
ENTRY(armv7_setttb)
	dsb
	mcr	p15, 0, r0, c2, c0,  0 /* Translation Table Base Register 0 */
	dsb
	mcr	p15, 0, r0, c7, c5,  6 /* Branch predictor invalidate all */
	dsb
	isb
	dsb
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 2 /* Invalidate unified TLB by ASID */
	dsb

	mov	pc, lr

/*
 * TLB functions
 */
ENTRY(armv7_tlb_flushID_SE)
	ldr	r1, .Lpage_mask
	bic	r0, r0, r1
#ifdef MULTIPROCESSOR
	mcr	p15, 0, r0, c8, c3, 3 /* Invalidate unified TLB by MVA, all ASID IS */
	mcr	p15, 0, r0, c7, c1,  6 /* Branch predictor invalidate all IS */
#else
	mcr	p15, 0, r0, c8, c7, 1 /* Invalidate unified TLB by MVA */
	mcr	p15, 0, r0, c7, c5,  6 /* Branch predictor invalidate all */
#endif
	dsb
	isb
	mov	pc, lr

/*
 * TLB functions
 */
ENTRY(armv7_tlb_flushID)
	dsb
#ifdef MULTIPROCESSOR
	mcr	p15, 0, r0, c8, c3, 0 /* Invalidate entire unified TLB IS */
	mcr	p15, 0, r0, c7, c1,  6 /* Branch predictor invalidate all IS */
#else
	mcr	p15, 0, r0, c8, c7, 0 /* Invalidate entire unified TLB */
	mcr	p15, 0, r0, c7, c5,  6 /* Branch predictor invalidate all */
#endif
	dsb
	isb
	mov	pc, lr

/* Based on algorithm from ARM Architecture Reference Manual */
ENTRY(armv7_dcache_wbinv_all)
	stmdb	sp!, {r4, r5, r6, r7, r8, r9}

	/* Get cache level */
	ldr	r0, .Lcoherency_level
	ldr	r3, [r0]
	cmp	r3, #0
	beq	Finished
	/* For each cache level */
	mov	r8, #0
Loop1:
	/* Get cache type for given level */
	mov	r2, r8, lsl #2
	add	r2, r2, r2
	ldr	r0, .Lcache_type
	ldr	r1, [r0, r2]

	/* Get line size */
	and	r2, r1, #7
	add	r2, r2, #4

	/* Get number of ways */
	ldr	r4, .Lway_mask
	ands	r4, r4, r1, lsr #3
	clz	r5, r4

	/* Get max index */
	ldr	r7, .Lmax_index
	ands	r7, r7, r1, lsr #13
Loop2:
	mov	r9, r4
Loop3:
	mov	r6, r8, lsl #1
	orr	r6, r6, r9, lsl r5
	orr	r6, r6, r7, lsl r2

	/* Clean and invalidate data cache by way/index */
	mcr	p15, 0, r6, c7, c14, 2
	subs	r9, r9, #1
	bge	Loop3
	subs	r7, r7, #1
	bge	Loop2
Skip:
	add	r8, r8, #1
	cmp	r3, r8
	bne Loop1
Finished:
	dsb
	ldmia	sp!, {r4, r5, r6, r7, r8, r9}
	mov	pc, lr

ENTRY(armv7_idcache_wbinv_all)
	stmdb	sp!, {lr}
	bl armv7_dcache_wbinv_all
#ifdef MULTIPROCESSOR
	mcr	p15, 0, r0, c7, c1,  0 /* Instruction cache invalidate all PoU, IS */
#else
	mcr	p15, 0, r0, c7, c5,  0 /* Instruction cache invalidate all PoU */
#endif
	dsb
	isb
	ldmia	sp!, {lr}
	mov	pc, lr

ENTRY(armv7_dcache_wb_range)
	ldr	ip, .Larmv7_dcache_line_size
	ldr	ip, [ip]
	sub	r3, ip, #1
	and	r2, r0, r3
	add	r1, r1, r2
	bic	r0, r0, r3
.Larmv7_wb_next:
	mcr	p15, 0, r0, c7, c10, 1 /* Data cache clean by MVA PoC */
	add	r0, r0, ip
	subs	r1, r1, ip
	bhi	.Larmv7_wb_next
	dsb				/* data synchronization barrier */
	mov	pc, lr

ENTRY(armv7_dcache_wbinv_range)
	ldr     ip, .Larmv7_dcache_line_size
	ldr     ip, [ip]
	sub     r3, ip, #1
	and     r2, r0, r3
	add     r1, r1, r2
	bic     r0, r0, r3
.Larmv7_wbinv_next:
	mcr	p15, 0, r0, c7, c14, 1 /* Data cache clean and invalidate by MVA PoC */
	add	r0, r0, ip
	subs	r1, r1, ip
	bhi	.Larmv7_wbinv_next
	dsb				/* data synchronization barrier */
	mov	pc, lr

/*
 * Note, we must not invalidate everything.  If the range is too big we
 * must use wb-inv of the entire cache.
 */
ENTRY(armv7_dcache_inv_range)
	ldr     ip, .Larmv7_dcache_line_size
	ldr     ip, [ip]
	sub     r3, ip, #1
	and     r2, r0, r3
	add     r1, r1, r2
	bic     r0, r0, r3
.Larmv7_inv_next:
	mcr	p15, 0, r0, c7, c6,  1 /* Data cache invalidate by MVA PoC */
	add	r0, r0, ip
	subs	r1, r1, ip
	bhi	.Larmv7_inv_next
	dsb				/* data synchronization barrier */
	mov	pc, lr

ENTRY(armv7_idcache_wbinv_range)
	ldr     ip, .Larmv7_idcache_line_size
	ldr     ip, [ip]
	sub     r3, ip, #1
	and     r2, r0, r3
	add     r1, r1, r2
	bic     r0, r0, r3
.Larmv7_id_wbinv_next:
	mcr	p15, 0, r0, c7, c5,  1 /* Instruction cache invalidate */
	mcr	p15, 0, r0, c7, c14, 1 /* Data cache clean and invalidate by MVA PoC */
	add	r0, r0, ip
	subs	r1, r1, ip
	bhi	.Larmv7_id_wbinv_next
	dsb				/* data synchronization barrier */
	isb				/* instruction synchronization barrier */
	mov	pc, lr

ENTRY_NP(armv7_icache_sync_all)
#ifdef MULTIPROCESSOR
	mcr	p15, 0, r0, c7, c1,  0 /* Instruction cache invalidate all PoU, IS */
#else
	mcr	p15, 0, r0, c7, c5,  0 /* Instruction cache invalidate all PoU */
#endif
	dsb				/* data synchronization barrier */
	isb				/* instruction synchronization barrier */
	mov	pc, lr

ENTRY_NP(armv7_icache_sync_range)
	ldr	ip, .Larmv7_icache_line_size
	ldr	ip, [ip]
.Larmv7_sync_next:
	mcr	p15, 0, r0, c7, c10, 1 /* Data cache clean by MVA PoC */
	mcr	p15, 0, r0, c7, c5,  1 /* Instruction cache invalidate */
	add	r0, r0, ip
	subs	r1, r1, ip
	bhi	.Larmv7_sync_next
	dsb				/* data synchronization barrier */
	isb				/* instruction synchronization barrier */
	mov	pc, lr

/*
 * Context switch.
 *
 * These is the CPU-specific parts of the context switcher cpu_switch()
 * These functions actually perform the TTB reload.
 *
 * NOTE: Special calling convention
 *	r1, r4-r13 must be preserved
 */
ENTRY(armv7_context_switch)
	dsb
	mcr	p15, 0, r0, c2, c0, 0 /* Translation Table Base Register 0 */
	isb
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 2 /* Invalidate unified TLB by ASID */
	mcr	p15, 0, r0, c7, c5, 6 /* Branch predictor invalidate all */
	dsb
	mov	pc, lr

/*
 * Invalidate all I+D+branch cache.  Used by startup code, which counts
 * on the fact that only r0-r3,ip are modified and no stack space is used.
 */
ENTRY(armv7_idcache_inv_all)
	mov     r0, #0
	mcr	p15, 2, r0, c0, c0,  0	@ set cache level to L1
	mrc	p15, 1, r0, c0, c0,  0	/* Cache Size ID Registers */

	ubfx    r2, r0, #13, #15        @ get num sets - 1 from CCSIDR
	ubfx    r3, r0, #3, #10         @ get numways - 1 from CCSIDR
	clz     r1, r3                  @ number of bits to MSB of way
	lsl     r3, r3, r1              @ shift into position
	mov     ip, #1                  @
	lsl     ip, ip, r1              @ ip now contains the way decr

	ubfx    r0, r0, #0, #3          @ get linesize from CCSIDR
	add     r0, r0, #4              @ apply bias
	lsl     r2, r2, r0              @ shift sets by log2(linesize)
	add     r3, r3, r2              @ merge numsets - 1 with numways - 1
	sub     ip, ip, r2              @ subtract numsets - 1 from way decr
	mov     r1, #1
	lsl     r1, r1, r0              @ r1 now contains the set decr
	mov     r2, ip                  @ r2 now contains set way decr

	/* r3 = ways/sets, r2 = way decr, r1 = set decr, r0 and ip are free */
1:	mcr	p15, 0, r3, c7, c6,  2		@ invalidate line
	movs    r0, r3                  @ get current way/set
	beq     2f                      @ at 0 means we are done.
	movs    r0, r0, lsl #10         @ clear way bits leaving only set bits
	subne   r3, r3, r1              @ non-zero?, decrement set #
	subeq   r3, r3, r2              @ zero?, decrement way # and restore set count
	b       1b

2:	dsb                             @ wait for stores to finish
	mov     r0, #0                  @ and ...
	mcr	p15, 0, r0, c7, c5,  0	@ invalidate instruction+branch cache
	isb                             @ instruction sync barrier
	bx      lr                      @ return
