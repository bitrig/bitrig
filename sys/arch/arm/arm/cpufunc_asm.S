/*	$OpenBSD: cpufunc_asm.S,v 1.2 2013/01/23 19:57:47 patrick Exp $	*/
/*	$NetBSD: cpufunc_asm.S,v 1.12 2003/09/06 09:14:52 rearnsha Exp $	*/

/*
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufunc.S 
 *
 * Assembly functions for CPU / MMU / TLB specific operations
 *
 * Created      : 30/01/97
 */
 
#include <arm/armreg.h>
#include <machine/asm.h>

	.text
	.align	2

ENTRY(cpufunc_nullop)
	mov	pc, lr

/*
 * Generic functions to read the internal coprocessor registers
 *
 * Currently these registers are :
 *  c0 - CPU ID
 *  c5 - Fault status
 *  c6 - Fault address
 *
 */

ENTRY(cpufunc_id)
	mrc	p15, 0, r0, c0, c0, 0
	mov	pc, lr

ENTRY(cpu_get_control)
	mrc	p15, 0, r0, c1, c0, 0
	mov	pc, lr

ENTRY(cpu_read_cache_config)
	mrc	p15, 0, r0, c0, c0, 1
	mov	pc, lr

ENTRY(cpufunc_dfsr)
	mrc	p15, 0, r0, c5, c0, 0
	mov	pc, lr

ENTRY(cpufunc_dfar)
	mrc	p15, 0, r0, c6, c0, 0
	mov	pc, lr

ENTRY(cpufunc_ifsr)
	mrc	p15, 0, r0, c5, c0, 1
	mov	pc, lr

ENTRY(cpufunc_ifar)
	mrc	p15, 0, r0, c6, c0, 2
	mov	pc, lr


/*
 * Generic functions to write the internal coprocessor registers
 *
 *
 * Currently these registers are 
 *  c1 - CPU Control
 *  c3 - Domain Access Control
 *
 * All other registers are CPU architecture specific
 */
 
#if 0 /* See below. */
ENTRY(cpufunc_control)
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
#endif

ENTRY(cpufunc_domains)
	mcr	p15, 0, r0, c3, c0, 0
	mov	pc, lr

/*
 * Generic functions to read/modify/write the internal coprocessor registers
 *
 *
 * Currently these registers are 
 *  c1 - CPU Control
 *
 * All other registers are CPU architecture specific
 */
 
ENTRY(cpufunc_control)
	mrc	p15, 0, r3, c1, c0, 0	/* Read the control register */
	bic	r2, r3, r0		/* Clear bits */
	eor     r2, r2, r1		/* XOR bits */

	teq	r2, r3			/* Only write if there is a change */
	mcrne	p15, 0, r2, c1, c0, 0	/* Write new control register */
	mov	r0, r3			/* Return old value */
	mov	pc, lr

/*
 * other potentially useful software functions are:
 *  clean D cache entry and flush I cache entry
 *   for the moment use cache_purgeID_E
 */

/* Random odd functions */

/*
 * Function to get the offset of a stored program counter from the
 * instruction doing the store.  This offset is defined to be the same
 * for all STRs and STMs on a given implementation.  Code based on
 * section 2.4.3 of the ARM ARM (2nd Ed.), with modifications to work
 * in 26-bit modes as well.
 */
ENTRY(get_pc_str_offset)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #4
	mov	r1, pc		/* R1 = addr of following STR */
	mov	r0, r0
	str	pc, [sp]	/* [SP] = . + offset */
	ldr	r0, [sp]
	sub	r0, r0, r1
	ldmdb	fp, {fp, sp, pc}
