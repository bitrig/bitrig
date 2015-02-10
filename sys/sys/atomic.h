/*	$OpenBSD: atomic.h,v 1.3 2015/02/10 11:39:18 dlg Exp $ */
/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
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

#ifndef _SYS_ATOMIC_H_
#define _SYS_ATOMIC_H_

#include <sys/stdatomic.h>
#include <machine/atomic.h>

/*
 * an arch wanting to provide its own implementations does so by defining
 * macros.
 */

/*
 * atomic_cas_*
 */

static inline unsigned int
atomic_cas_uint(volatile unsigned int *p, unsigned int o, unsigned int n)
{
	atomic_compare_exchange_weak_explicit((atomic_uint *)p, &o, n,
	    memory_order_relaxed, memory_order_relaxed);
	return o;
}

static inline unsigned long
atomic_cas_ulong(volatile unsigned long *p, unsigned long o, unsigned long n)
{
	atomic_compare_exchange_weak_explicit((atomic_ulong *)p, &o, n,
	    memory_order_relaxed, memory_order_relaxed);
	return o;
}

static inline void *
atomic_cas_ptr(volatile void *p, void *o, void *n)
{
	atomic_compare_exchange_weak_explicit((atomic_uintptr_t *)p,
	    (__uintptr_t *)&o, (__uintptr_t)n,
	    memory_order_relaxed, memory_order_relaxed);
	return o;
}

/*
 * atomic_swap_*
 */

static inline unsigned int
atomic_swap_uint(volatile unsigned int *p, unsigned int v)
{
	return atomic_exchange_explicit((atomic_uint *)p, v,
	    memory_order_relaxed);
}

static inline unsigned long
atomic_swap_ulong(volatile unsigned long *p, unsigned long v)
{
	return atomic_exchange_explicit((atomic_ulong *)p, v,
	    memory_order_relaxed);
}

static inline void *
atomic_swap_ptr(volatile void *p, void *v)
{
	return (void *)atomic_exchange_explicit((atomic_uintptr_t *)p,
	    (__uintptr_t)v, memory_order_relaxed);
}

/*
 * atomic_add_*_nv - add and fetch
 */

static inline unsigned int
atomic_add_int_nv(volatile unsigned int *p, unsigned int v)
{
	return atomic_fetch_add_explicit((atomic_uint *)p, v,
	    memory_order_relaxed) + v;
}

static inline unsigned long
atomic_add_long_nv(volatile unsigned long *p, unsigned long v)
{
	return atomic_fetch_add_explicit((atomic_ulong *)p, v,
	    memory_order_relaxed) + v;
}

/*
 * atomic_add - add
 */

#define atomic_add_int(_p, _v) ((void)atomic_add_int_nv((_p), (_v)))
#define atomic_add_long(_p, _v) ((void)atomic_add_long_nv((_p), (_v)))

/*
 * atomic_inc_*_nv - increment and fetch
 */

#define atomic_inc_int_nv(_p) atomic_add_int_nv((_p), 1)
#define atomic_inc_long_nv(_p) atomic_add_long_nv((_p), 1)

/*
 * atomic_inc_* - increment
 */

#define atomic_inc_int(_p) ((void)atomic_inc_int_nv(_p))

#define atomic_inc_long(_p) ((void)atomic_inc_long_nv(_p))

/*
 * atomic_sub_*_nv - sub and fetch
 */

static inline unsigned int
atomic_sub_int_nv(volatile unsigned int *p, unsigned int v)
{
	return atomic_fetch_sub_explicit((atomic_uint *)p, v,
	    memory_order_relaxed) + v;
}

static inline unsigned long
atomic_sub_long_nv(volatile unsigned long *p, unsigned long v)
{
	return atomic_fetch_sub_explicit((atomic_ulong *)p, v,
	    memory_order_relaxed) + v;
}

/*
 * atomic_sub_* - sub
 */

#define atomic_sub_int(_p, _v) ((void)atomic_sub_int_nv((_p), (_v)))
#define atomic_sub_long(_p, _v) ((void)atomic_sub_long_nv((_p), (_v)))

/*
 * atomic_dec_*_nv - decrement and fetch
 */

#define atomic_dec_int_nv(_p) atomic_sub_int_nv((_p), 1)
#define atomic_dec_long_nv(_p) atomic_sub_long_nv((_p), 1)

/*
 * atomic_dec_* - decrement
 */

#define atomic_dec_int(_p) ((void)atomic_dec_int_nv(_p))
#define atomic_dec_long(_p) ((void)atomic_dec_long_nv(_p))

#endif /* _SYS_ATOMIC_H_ */
