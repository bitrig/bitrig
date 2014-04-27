/*	$OpenBSD: mutex.h,v 1.7 2009/08/13 13:24:55 weingart Exp $	*/

/*
 * Copyright (c) 2004 Artur Grabowski <art@openbsd.org>
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <sys/stdatomic.h>

/*
 * A mutex is:
 *  - owned by a cpu.
 *  - non-recursive.
 *  - spinning.
 *  - not providing mutual exclusion between processes, only cpus.
 *  - providing interrupt blocking when necessary.
 *
 * Different mutexes can be nested, but not interleaved. This is ok:
 * "mtx_enter(foo); mtx_enter(bar); mtx_leave(bar); mtx_leave(foo);"
 * This is _not_ ok:
 * "mtx_enter(foo); mtx_enter(bar); mtx_leave(foo); mtx_leave(bar);"
 */

struct mutex {
	void	*mtx_owner;		/* Current CPU owning lock. */
	int	 mtx_wantipl;		/* IPL while locked. */
	int	 mtx_oldipl;		/* IPL prior to locking. */
	atomic_int mtx_ticket;		/* First available ticket. */
	atomic_int mtx_cur;		/* Active ticket. */
};

__BEGIN_DECLS
void mtx_init(struct mutex *, int);
void mtx_enter(struct mutex *);
void mtx_leave(struct mutex *);
int mtx_enter_try(struct mutex *);
__END_DECLS

/*
 * To prevent lock ordering problems with the kernel lock, we need to
 * make sure we block all interrupts that can grab the kernel lock.
 * The simplest way to achieve this is to make sure mutexes always
 * raise the interrupt ptiority level to the highest level that has
 * interrupts that grab the kernel lock.
 */
#ifdef MULTIPROCESSOR
#define __MUTEX_IPL(ipl) \
    (((ipl) > IPL_NONE && (ipl) < IPL_TTY) ? IPL_TTY : (ipl))
#else
#define __MUTEX_IPL(ipl) (ipl)
#endif

#define MUTEX_INITIALIZER(ipl)						\
	{								\
		NULL,							\
		__MUTEX_IPL((ipl)),					\
		IPL_NONE,						\
	}

#define MUTEX_OLDIPL(mtx)	((mtx)->mtx_oldipl)

#define MUTEX_ASSERT_LOCKED(mtx)					\
	do {								\
		if ((mtx)->mtx_owner != curcpu())			\
			panic("mutex %p not held in %s:%d",		\
			    (mtx), __func__, __LINE__);			\
	} while (0)

#define MUTEX_ASSERT_UNLOCKED(mtx)					\
	do {								\
		if ((mtx)->mtx_owner == curcpu())			\
			panic("mutex %p held in %s:%d",			\
			    (mtx), __func__, __LINE__);			\
	} while (0)

#endif
