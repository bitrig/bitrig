/*	$OpenBSD: setjmp.S,v 1.3 2015/09/13 07:36:58 guenther Exp $	*/
/*	$NetBSD: setjmp.S,v 1.5 2003/04/05 23:08:51 bjh21 Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <machine/setjmp.h>

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from the last call to
 *	setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is restored.
 */

ENTRY(setjmp)
	/* Block all signals and retrieve the old signal mask */
	stmfd	sp!, {r0, lr}
	mov	r0, #0x00000000

#if defined(__PIC__)
	bl	_C_LABEL(_libc_sigblock)(PLT)
#else
	bl	_C_LABEL(_libc_sigblock)
#endif
	mov	r1, r0

	ldmfd	sp!, {r0, lr}

	/* Store signal mask */
	str	r1, [r0, #(25 * 4)]

	ldr	r1, .Lsetjmp_magic
	str	r1, [r0]

#ifndef SOFTFLOAT
	/* Store fp registers */
	add	r1, r0, #(_JB_REG_D8 * 4)
	vstmia	r1, {d8-d15}
	/* Store fpscr */
	vmrs	r1, FPSCR
	str	r1, [r0, #(_JB_REG_FPSCR * 4)]
#endif	/*SOFTFLOAT*/

	/* Store integer registers */
	add	r0, r0, #(_JB_REG_R4 * 4)
	stmia	r0, {r4-lr}
	mov	r0, #0x00000000
	mov	pc, lr

.Lsetjmp_magic:
	.word	_JB_MAGIC_SETJMP


ENTRY(longjmp)
	ldr	r2, .Lsetjmp_magic
	ldr	r3, [r0]
	teq	r2, r3
	bne	botch

	/* Fetch signal mask */
	ldr	r2, [r0, #(25 * 4)]

	/* Set signal mask */
	stmfd	sp!, {r0, r1, lr}
	sub	sp, sp, #4	/* align the stack */

	mov	r0, r2
#if defined(__PIC__)
	bl	_C_LABEL(_libc_sigsetmask)(PLT)
#else
	bl	_C_LABEL(_libc_sigsetmask)
#endif

	add	sp, sp, #4	/* unalign the stack */
	ldmfd	sp!, {r0, r1, lr}

#ifndef SOFTFLOAT
	/* Restore fp registers */
	add	ip, r0, #(_JB_REG_D8 * 4)
	vldmia	ip, {d8-d15}
	/* Restore fpscr */
	ldr	ip, [r0, #(_JB_REG_FPSCR * 4)]
	vmsr	FPSCR, ip
#endif	/* SOFTFLOAT */

	/* Restore integer registers */
	add	r0, #(_JB_REG_R4 * 4)
	ldmia	r0, {r4-lr}

	/* Validate sp and lr */
	teq	sp, #0
	teqne	lr, #0
	beq	botch

	/* Set return value */

	mov	r0, r1
	teq	r0, #0x00000000
	moveq	r0, #0x00000001
	mov	pc, lr

	/* validation failed, die die die. */
botch:
#if defined(__PIC__)
	bl	_C_LABEL(longjmperror)(PLT)
	bl	_C_LABEL(abort)(PLT)
#else
	bl	_C_LABEL(longjmperror)
	bl	_C_LABEL(abort)
#endif
	b	. - 8		/* Cannot get here */
