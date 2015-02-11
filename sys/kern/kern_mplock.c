/*	$OpenBSD: lock_machdep.c,v 1.7 2015/02/11 06:39:24 dlg Exp $	*/

/*
 * Copyright (c) 2007 Artur Grabowski <art@openbsd.org>
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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mplock.h>

#include <ddb/db_output.h>

void
__mp_lock_init(struct __mp_lock *mpl)
{
	bzero(mpl->mpl_cpus, sizeof(mpl->mpl_cpus));
	mpl->mpl_ticket = 0;
	atomic_init(&mpl->mpl_users, 0);
}

#if defined(MP_LOCKDEBUG)
#ifndef DDB
#error "MP_LOCKDEBUG requires DDB"
#endif

/* CPU-dependent timing, needs this to be settable from ddb. */
extern int __mp_lock_spinout;
#endif

static __inline void
__mp_lock_spin(struct __mp_lock *mpl, u_int me)
{
#ifndef MP_LOCKDEBUG
	while (mpl->mpl_ticket != me)
		SPINWAIT();
#else
	int ticks = __mp_lock_spinout;

	while (mpl->mpl_ticket != me && --ticks > 0)
		SPINWAIT();

	if (ticks == 0) {
		db_printf("__mp_lock(%p): lock spun out\n", mpl);
		Debugger();
	}
#endif
}

void
__mp_lock(struct __mp_lock *mpl)
{
	intr_state_t its;
	struct __mp_lock_cpu *cpu = &mpl->mpl_cpus[cpu_number()];

	its = intr_disable();
	if (cpu->mplc_depth++ == 0)
		cpu->mplc_ticket = atomic_fetch_add_explicit(&mpl->mpl_users,
		    1, memory_order_acquire);
	intr_restore(its);

	__mp_lock_spin(mpl, cpu->mplc_ticket);
}

void
__mp_unlock(struct __mp_lock *mpl)
{
	intr_state_t its;
	struct __mp_lock_cpu *cpu = &mpl->mpl_cpus[cpu_number()];

#ifdef MP_LOCKDEBUG
	if (!__mp_lock_held(mpl)) {
		db_printf("__mp_unlock(%p): not held lock\n", mpl);
		Debugger();
	}
#endif

	its = intr_disable();
	if (--cpu->mplc_depth == 0) {
		mpl->mpl_ticket++;
		SPINWAKE();
	}
	intr_restore(its);
}

int
__mp_release_all(struct __mp_lock *mpl)
{
	struct __mp_lock_cpu *cpu = &mpl->mpl_cpus[cpu_number()];
	int rv = cpu->mplc_depth;
	intr_state_t its;

	its = intr_disable();
	cpu->mplc_depth = 0;
	mpl->mpl_ticket++;
	SPINWAKE();
	intr_restore(its);

	return (rv);
}

int
__mp_release_all_but_one(struct __mp_lock *mpl)
{
	struct __mp_lock_cpu *cpu = &mpl->mpl_cpus[cpu_number()];
	int rv = cpu->mplc_depth - 1;

#ifdef MP_LOCKDEBUG
	if (!__mp_lock_held(mpl)) {
		db_printf("__mp_release_all_but_one(%p): not held lock\n", mpl);
		Debugger();
	}
#endif

	cpu->mplc_depth = 1;

	return (rv);
}

void
__mp_acquire_count(struct __mp_lock *mpl, int count)
{
	while (count--)
		__mp_lock(mpl);
}

int
__mp_lock_held(struct __mp_lock *mpl)
{
	struct __mp_lock_cpu *cpu = &mpl->mpl_cpus[cpu_number()];

	return (cpu->mplc_ticket == mpl->mpl_ticket && cpu->mplc_depth > 0);
}
