/*	$OpenBSD: cpuswitch7.S,v 1.1 2013/04/30 13:04:25 patrick Exp $	*/
/*	$NetBSD: cpuswitch.S,v 1.41 2003/11/15 08:44:18 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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
 * cpuswitch.S
 *
 * cpu switching functions
 *
 * Created      : 15/10/94
 */

#include "assym.h"
#include <arm/armreg.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/asm.h>

/* LINTSTUB: include <sys/param.h> */
	
/*
 * These are used for switching the translation table/DACR.
 * Since the vector page can be invalid for a short time, we must
 * disable both regular IRQs *and* FIQs.
 *
 * XXX: This is not necessary if the vector table is relocated.
 */
#define IRQdisableALL \
	mrs	r14, cpsr ; \
	orr	r14, r14, #(I32_bit | F32_bit) ; \
	msr	cpsr_c, r14

#define IRQenableALL \
	mrs	r14, cpsr ; \
	bic	r14, r14, #(I32_bit | F32_bit) ; \
	msr	cpsr_c, r14

	.text

.Lcpufuncs:
	.word	_C_LABEL(cpufuncs)

.Lcpu_do_powersave:
	.word	_C_LABEL(cpu_do_powersave)

/*
 * Idle loop, exercised while waiting for a process to wake up.
 */
ENTRY(cpu_idle_enter)
	mov	pc, lr

ENTRY(cpu_idle_cycle)
	stmfd	sp!, {r6, lr}

	ldr	r6, .Lcpu_do_powersave
	ldr	r6, [r6]		/* r6 = cpu_do_powersave */

	teq	r6, #0			/* cpu_do_powersave non zero? */
	ldrne	r6, .Lcpufuncs
	ldrne	r6, [r6, #(CF_SLEEP)]

	teq	r6, #0			/* Powersave idle? */
	beq	.Lidle_return		/* Nope. Just continue. */

	/*
	 * Before going into powersave idle mode, disable interrupts.
	 */
	IRQdisableALL
	mov	lr, pc
	mov	pc, r6			/* If so, do powersave idle */
	IRQenableALL

.Lidle_return:
	ldmfd	sp!, {r6, pc}

ENTRY(cpu_idle_leave)
	mov	pc, lr


/*
 * cpu_switchto(struct proc *oldproc, struct proc *newproc)
 *
 * Performs a process context switch from oldproc (which may be NULL)
 * to newproc.
 *
 * Arguments:
 *	r0	'struct proc *' of the context to switch from
 *	r1	'struct proc *' of the context to switch to
 */

ENTRY(cpu_switchto)
	stmfd	sp!, {r4-r7, lr}
	sub	sp, sp, #4

	/* Get curcpu from TPIDRPRW. */
	mrc	p15, 0, r3, c13, c0, 4
#ifdef MULTIPROCESSOR
	str	r3, [r1, #(P_CPU)]
#else
	/* p->p_cpu initialized in fork1() for single-processor */
#endif

	/* Process is now on a processor. */
	mov	r2, #SONPROC			/* p->p_stat = SONPROC */
	strb	r2, [r1, #(P_STAT)]

	/* We have a new curproc now so make a note it */
	str	r1, [r3, #(CI_CURPROC)]

	/* Hook in a new pcb */
	ldr	r6, [r3, #(CI_CURPCB)]		/* Remember the old PCB */
	ldr	r2, [r1, #(P_ADDR)]
	str	r2, [r3, #(CI_CURPCB)]

	/*
	 * If the old proc on entry to cpu_switch was zero then the
	 * process that called it was exiting. This means that we do
	 * not need to save the current context (we nevertheless need
	 * to clear the cache and TLB).
	 * XXX: Store r4-r12, sp, lr, pc?
	 */
	teq	r0, #0x00000000
	beq	.Lswitch_exited

	/* Save all the registers in the old proc's pcb */
	add	r7, r6, #(PCB_R8)
	stmia	r7, {r8-r13}

.Lswitch_exited:
	/*
	 * NOTE: We can now use r8-r13 until it is time to restore
	 * them for the new process.
	 */

	/* Remember the old PCB. */
	mov	r8, r6

	/* Save new proc in r6 now. */
	mov	r6, r1

	/* Get the user structure for the new process in r9 */
	ldr	r9, [r6, #(P_ADDR)]

	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
	mrs	r3, cpsr
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE | I32_bit)
	msr	cpsr_c, r2

#ifdef notworthit
	teq	r0, #0x00000000
	strne	sp, [r8, #(PCB_UND_SP)]
#else
	str	sp, [r8, #(PCB_UND_SP)]
#endif

	msr	cpsr_c, r3		/* Restore the old mode */

	/* rem: r0 = old proc */
	/* rem: r1 = r6 = new process */
	/* rem: r8 = old PCB */
	/* rem: r9 = new PCB */

	/* What else needs to be saved  Only FPA stuff when that is supported */

	/* Third phase: restore saved context */

	/*
	 * Get the new L1 table pointer into r11.  If we're switching to
	 * an LWP with the same address space as the outgoing one, we can
	 * skip the cache purge and the TTB load.
	 *
	 * To avoid data dep stalls that would happen anyway, we try
	 * and get some useful work done in the mean time.
	 */
	ldr	r10, [r8, #(PCB_PAGEDIR)]	/* r10 = old L1 */
	ldr	r11, [r9, #(PCB_PAGEDIR)]	/* r11 = new L1 */

	teq	r10, r11			/* Same L1? */
	beq	.Lcs_context_switched		/* yes! */

	/*
	 * Do a full context switch, including full TLB flush.
	 */
	ldr	r10, .Lcpufuncs
	mov	r0, r11
	mov	lr, pc
	ldr	pc, [r10, #CF_CONTEXT_SWITCH]

.Lcs_context_switched:

	/* XXXSCW: Safe to re-enable FIQs here */

	/* rem: r6 = new proc */
	/* rem: r9 = new PCB */

	/* stick tcb into user visable register (V7 only) */
	ldr	r3, [r9, #(PCB_TCB)]
	mcr	p15, 0, r3, c13, c0, 3

	/*
	 * This can be optimised... We know we want to go from SVC32
	 * mode to UND32 mode
	 */
	mrs	r3, cpsr
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, #(PSR_UND32_MODE)
	msr	cpsr_c, r2

	ldr	sp, [r9, #(PCB_UND_SP)]

	msr	cpsr_c, r3		/* Restore the old mode */

	/* Restore all the save registers */
	add	r7, r9, #PCB_R8
	ldmia	r7, {r8-r13}

	/* We can enable interrupts again */
	IRQenableALL

	add	sp, sp, #4
	ldmfd	sp!, {r4-r7, pc}

/* LINTSTUB: Func: void savectx(struct pcb *pcb) */
ENTRY(savectx)
	/*
	 * r0 = pcb
	 */

	/* Push registers.*/
	stmfd	sp!, {r4-r7, lr}
	sub	sp, sp, #4

	/* Store all the registers in the process's pcb */
	add	r2, r0, #(PCB_R8)
	stmia	r2, {r8-r13}

	/* Pull the regs of the stack */
	add	sp, sp, #4
	ldmfd	sp!, {r4-r7, pc}

ENTRY(proc_trampoline)
#ifdef MULTIPROCESSOR
	bl	_C_LABEL(proc_trampoline_mp)
#else
	mov	r0, #(IPL_NONE)
	bl	_C_LABEL(_spllower)
#endif

	mov	r0, r5
	mov	r1, sp
	mov	lr, pc
	mov	pc, r4

	/* Kill irq's */
        mrs     r0, cpsr
        orr     r0, r0, #(I32_bit)
        msr     cpsr_c, r0

	PULLFRAME

	movs	pc, lr			/* Exit */
