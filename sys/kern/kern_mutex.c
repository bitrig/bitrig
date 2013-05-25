/*
 * Copyright (c) 2012 Ariane van der Steldt <ariane@stack.nl>
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
#include <sys/mutex.h>
#include <sys/proc.h>
#include <machine/intr.h>


void
mtx_init(struct mutex *mtx, int ipl)
{
	mtx->mtx_owner = NULL;
	atomic_init(&mtx->mtx_ticket, 0);
	atomic_init(&mtx->mtx_cur, 0);
}

/* Store old ipl and current owner in mutex. */
static __inline void
mtx_store(struct mutex *mtx)
{
	/* Ensure mutex is ownerless and valid. */
	KASSERT(mtx->mtx_owner == NULL);

	mtx->mtx_owner = curcpu();
}

/* Load owner from mutex. */
static __inline void
mtx_clear(struct mutex *mtx)
{
	/* Ensure we own the mutex. */
	KASSERT(mtx->mtx_owner == curcpu());

	mtx->mtx_owner = NULL;
}

void
mtx_enter(struct mutex *mtx)
{
	unsigned int t;

	/* Block interrupts. */
	crit_enter();

	/* Ensure we're not holding the mutex already. */
	KASSERT(mtx->mtx_owner != curcpu());

	/* Acquire ticket. */
	t = atomic_fetch_add_explicit(&mtx->mtx_ticket,
	    1, memory_order_acquire);
	/* Wait for our turn. */
	while (atomic_load_explicit(&mtx->mtx_cur, memory_order_acquire) != t)
		SPINWAIT();

	/* Save mutex. */
	mtx_store(mtx);

#ifdef DIAGNOSTIC
	/* Increase mutex level. */
	curcpu()->ci_mutex_level++;
#endif
}

int
mtx_enter_try(struct mutex *mtx)
{
	unsigned int t;

	/* Block interrupts. */
	crit_enter();

	/* Ensure we're not holding the mutex already. */
	KASSERT(mtx->mtx_owner != curcpu());

	/* Figure out which ticket is active. */
	t = atomic_load_explicit(&mtx->mtx_cur, memory_order_relaxed);
	/* Take ticket t, if it is available. */
	if (atomic_compare_exchange_strong_explicit(&mtx->mtx_ticket, &t, t + 1,
	    memory_order_acquire, memory_order_relaxed)) {
		/* We hold the lock, save owner. */
		mtx_store(mtx);

#ifdef DIAGNOSTIC
		/* Increase mutex level. */
		curcpu()->ci_mutex_level++;
#endif

		return 1;
	}

	/* Lock is unavailable. */
	crit_leave();
	return 0;
}

void
mtx_leave(struct mutex *mtx)
{
#ifdef DIAGNOSTIC
	/* Decrease mutex level. */
	curcpu()->ci_mutex_level--;
#endif

	/* Clear the mutex. */
	mtx_clear(mtx);

	/* Pass mutex to next-in-line. */
	atomic_fetch_add_explicit(&mtx->mtx_cur, 1, memory_order_release);

	/* Wake up the waiters. */
	SPINWAKE();

	/* Clear interrupt block. */
	crit_leave();
}
