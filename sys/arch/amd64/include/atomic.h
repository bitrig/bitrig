/*	$OpenBSD: atomic.h,v 1.11 2014/03/27 10:24:40 dlg Exp $	*/
/*	$NetBSD: atomic.h,v 1.1 2003/04/26 18:39:37 fvdl Exp $	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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

#ifndef _MACHINE_ATOMIC_H_
#define _MACHINE_ATOMIC_H_

/*
 * Perform atomic operations on memory. Should be atomic with respect
 * to interrupts and multiple processors.
 *
 * void atomic_setbits_int(volatile u_int *a, u_int mask) { *a |= mask; }
 * void atomic_clearbits_int(volatile u_int *a, u_int mas) { *a &= ~mask; }
 */

#if defined(_KERNEL) && !defined(_LOCORE)

#ifdef MULTIPROCESSOR
#define LOCK "lock"
#else
#define LOCK
#endif

static inline unsigned int
_atomic_cas_uint(volatile unsigned int *p, unsigned int e, unsigned int n)
{
	__asm __volatile(LOCK " cmpxchgl %2, %1"
	    : "=a" (n), "=m" (*p)
	    : "r" (n), "a" (e), "m" (*p)
	    : "memory");

	return (n);
}
#define atomic_cas_uint(_p, _e, _n) _atomic_cas_uint((_p), (_e), (_n))

static inline unsigned long
_atomic_cas_ulong(volatile unsigned long *p, unsigned long e, unsigned long n)
{
	__asm __volatile(LOCK " cmpxchgq %2, %1"
	    : "=a" (n), "=m" (*p)
	    : "r" (n), "a" (e), "m" (*p)
	    : "memory");

	return (n);
}
#define atomic_cas_ulong(_p, _e, _n) _atomic_cas_ulong((_p), (_e), (_n))

static inline void *
_atomic_cas_ptr(volatile void **p, void *e, void *n)
{
	__asm __volatile(LOCK " cmpxchgq %2, %1"
	    : "=a" (n), "=m" (*p)
	    : "r" (n), "a" (e), "m" (*p)
	    : "memory");

	return (n);
}
#define atomic_cas_ptr(_p, _e, _n) _atomic_cas_ptr((_p), (_e), (_n))

static __inline u_int64_t
x86_atomic_testset_u64(volatile u_int64_t *ptr, u_int64_t val)
{
	__asm__ volatile ("xchgq %0,(%2)" :"=r" (val):"0" (val),"r" (ptr));
	return val;
}

static __inline u_int32_t
x86_atomic_testset_u32(volatile u_int32_t *ptr, u_int32_t val)
{
	__asm__ volatile ("xchgl %0,(%2)" :"=r" (val):"0" (val),"r" (ptr));
	return val;
}


static __inline void
x86_atomic_setbits_u32(volatile u_int32_t *ptr, u_int32_t bits)
{
	__asm __volatile(LOCK " orl %1,%0" :  "=m" (*ptr) : "ir" (bits));
}

static __inline void
x86_atomic_clearbits_u32(volatile u_int32_t *ptr, u_int32_t bits)
{
	__asm __volatile(LOCK " andl %1,%0" :  "=m" (*ptr) : "ir" (~bits));
}

/*
 * XXX XXX XXX
 * theoretically 64bit cannot be used as
 * an "i" and thus if we ever try to give
 * these anything from the high dword there
 * is an asm error pending
 */
static __inline void
x86_atomic_setbits_u64(volatile u_int64_t *ptr, u_int64_t bits)
{
	__asm __volatile(LOCK " orq %1,%0" :  "=m" (*ptr) : "ir" (bits));
}

static __inline void
x86_atomic_clearbits_u64(volatile u_int64_t *ptr, u_int64_t bits)
{
	__asm __volatile(LOCK " andq %1,%0" :  "=m" (*ptr) : "ir" (~bits));
}

#define x86_atomic_testset_ul	x86_atomic_testset_u64
#define x86_atomic_setbits_ul	x86_atomic_setbits_u64
#define x86_atomic_clearbits_ul	x86_atomic_clearbits_u64

#define atomic_setbits_int x86_atomic_setbits_u32
#define atomic_clearbits_int x86_atomic_clearbits_u32

static inline void
atomic_add_u32(volatile uint32_t *up, uint32_t v)
{
	__asm __volatile(LOCK " addl %1,%0" : "=m" (*up) : "ir" (v));
}

static inline void
atomic_sub_u32(volatile uint32_t *up, uint32_t v)
{
	__asm __volatile(LOCK " subl %1,%0" : "=m" (*up) : "ir" (v));
}

static inline void
atomic_add_32(volatile int32_t *p, int32_t v)
{
	atomic_add_u32((volatile uint32_t *)p, v);
}

static inline void
atomic_sub_32(volatile int32_t *p, int32_t v)
{
	atomic_sub_u32((volatile uint32_t *)p, v);
}

static inline void
atomic_add_u64(volatile uint64_t *up, uint64_t v)
{
	__asm __volatile(LOCK " addq %1,%0" : "=m" (*up) : "ir" (v));
}

static inline void
atomic_sub_u64(volatile uint64_t *up, uint64_t v)
{
	__asm __volatile(LOCK " subq %1,%0" : "=m" (*up) : "ir" (v));
}

static inline void
atomic_add_64(volatile int64_t *p, int64_t v)
{
	atomic_add_u64((volatile uint64_t *)p, v);
}

static inline void
atomic_sub_64(volatile int64_t *p, int64_t v)
{
	atomic_sub_u64((volatile uint64_t *)p, v);
}

/* Increment/decrement operations - MD. */
static inline void
atomic_inc_u32(volatile uint32_t *up)
{
	atomic_add_u32(up, 1);
}

static inline void
atomic_dec_u32(volatile uint32_t *up)
{
	atomic_sub_u32(up, 1);
}

static inline void
atomic_inc_32(volatile int32_t *p)
{
	atomic_add_32((volatile int32_t *)p, 1);
}

static inline void
atomic_dec_32(volatile int32_t *p)
{
	atomic_sub_32((volatile int32_t *)p, 1);
}

static inline void
atomic_inc_u64(volatile uint64_t *up)
{
	atomic_add_u64(up, 1);
}

static inline void
atomic_dec_u64(volatile uint64_t *up)
{
	atomic_sub_u64(up, 1);
}

static inline void
atomic_inc_64(volatile int64_t *p)
{
	atomic_add_64((volatile int64_t *)p, 1);
}

static inline void
atomic_dec_64(volatile int64_t *p)
{
	atomic_sub_64((volatile int64_t *)p, 1);
}

/* Increment/decrement int/long - exported interface. */
static inline void
atomic_inc_uint(volatile unsigned int *up)
{
	atomic_inc_u32((volatile uint32_t *)up);
}

static inline void
atomic_dec_uint(volatile unsigned int *up)
{
	atomic_dec_u32((volatile uint32_t *)up);
}

static inline void
atomic_inc_int(volatile int *p)
{
	atomic_inc_32((volatile int32_t *)p);
}

static inline void
atomic_dec_int(volatile int *p)
{
	atomic_dec_32((volatile int32_t *)p);
}

static inline void
atomic_inc_ulong(volatile unsigned long *up)
{
	atomic_inc_u64((volatile uint64_t *)up);
}

static inline void
atomic_dec_ulong(volatile unsigned long *up)
{
	atomic_dec_u64((volatile uint64_t *)up);
}

static inline void
atomic_inc_long(volatile unsigned long *p)
{
	atomic_inc_64((volatile int64_t *)p);
}

static inline void
atomic_dec_long(volatile unsigned long *p)
{
	atomic_dec_64((volatile int64_t *)p);
}

/* Add/Sub int/long - exported interface. */
static inline void
atomic_add_uint(volatile unsigned int *up, unsigned int v)
{
	atomic_add_u32((volatile uint32_t *)up, v);
}

static inline void
atomic_sub_uint(volatile unsigned *up, unsigned int v)
{
	atomic_sub_u32((volatile uint32_t *)up, v);
}

static inline void
atomic_add_int(volatile int *p, int v)
{
	atomic_add_32((volatile int32_t *)p, v);
}

static inline void
atomic_sub_int(volatile int *p, int v)
{
	atomic_sub_32((volatile int32_t *)p, v);
}

static inline void
atomic_add_ulong(volatile unsigned long *up, unsigned long v)
{
	atomic_add_u64((volatile uint64_t *)up, v);
}

static inline void
atomic_sub_ulong(volatile unsigned long *up, unsigned long v)
{
	atomic_sub_u64((volatile uint64_t *)up, v);
}

static inline void
atomic_add_long(volatile long *p, unsigned long v)
{
	atomic_add_64((volatile int64_t *)p, v);
}

static inline void
atomic_sub_long(volatile long *p, unsigned long v)
{
	atomic_sub_64((volatile int64_t *)p, v);
}

#undef LOCK

#endif /* defined(_KERNEL) && !defined(_LOCORE) */
#endif /* _MACHINE_ATOMIC_H_ */
