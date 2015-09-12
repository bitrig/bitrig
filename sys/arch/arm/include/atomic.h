/*	$OpenBSD: atomic.h,v 1.12 2015/09/12 16:12:50 miod Exp $	*/

/* Public Domain */

#ifndef _ARM_ATOMIC_H_
#define _ARM_ATOMIC_H_

#if defined(_KERNEL)

#include <arm/cpufunc.h>
#include <arm/armreg.h>

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

#if defined(CPU_ARMv7)
#define __membar(_f) do { __asm __volatile(_f ::: "memory"); } while (0)

#define membar_enter()		__membar("dmb sy")
#define membar_exit()		__membar("dmb sy")
#define membar_producer()	__membar("dmb st")
#define membar_consumer()	__membar("dmb sy")
#define membar_sync()		__membar("dmb sy")

#define virtio_membar_producer()	__membar("dmb st")
#define virtio_membar_consumer()	__membar("dmb sy")
#define virtio_membar_sync()		__membar("dmb sy")
#endif /* CPU_ARMv7 */

#endif /* defined(_KERNEL) */
#endif /* _ARM_ATOMIC_H_ */
