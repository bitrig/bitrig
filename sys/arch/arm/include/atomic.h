/*	$OpenBSD: atomic.h,v 1.10 2014/11/14 09:56:06 dlg Exp $	*/

/* Public Domain */

#ifndef _ARM_ATOMIC_H_
#define _ARM_ATOMIC_H_

#if defined(_KERNEL)

#include <sys/stdatomic.h>
#include <arm/cpufunc.h>

/*
 * on pre-v6 arm processors, it is necessary to disable interrupts if
 * in the kernel and atomic updates are necessary without full mutexes
 */

/*
 * Use stdatomic instructions instead of encoding ASM.
 */
static inline void
atomic_setbits_int(volatile unsigned int *uip, unsigned int v)
{
	atomic_fetch_or_explicit((atomic_uint *)uip, v, memory_order_seq_cst);
}

static inline void
atomic_clearbits_int(volatile unsigned int *uip, unsigned int v)
{
	atomic_fetch_and_explicit((atomic_uint *)uip, ~v, memory_order_seq_cst);
}

#endif /* defined(_KERNEL) */
#endif /* _ARM_ATOMIC_H_ */
