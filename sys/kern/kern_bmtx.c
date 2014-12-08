 /*
  * Copyright (c) 2014 Christiano F. Haesbaert <haesbaert@haesbaert.org>
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
#include <sys/proc.h>
#include <sys/stdatomic.h>
#include <sys/stdint.h>
#include <sys/bmtx.h>

void	bmtx_lock_block(struct bmtx *, uintptr_t);

/* #define BMTX_STATS */
#ifdef BMTX_STATS
#define BST(x) (x++)
uint64_t bmtx_fast_locks;
uint64_t bmtx_fast_unlocks;
uint64_t bmtx_recursed_locks;
uint64_t bmtx_recursed_unlocks;
uint64_t bmtx_lock_blocks;
uint64_t bmtx_lock_blocks_cancelled;
uint64_t bmtx_lock_blocks_slept;
uint64_t bmtx_spun_locks;
uint64_t bmtx_blocked_unlocks;
#else
#define BST(x)
#endif
/*
 * XXX big design issue, interlock will HAVE to be schedlock, since we test for
 * p->p_stat.
 */
#define bmtx_inter_lock(b)	SCHED_LOCK()
#define bmtx_inter_unlock(b)	SCHED_UNLOCK()
#define bmtx_cmp_set(b,o,n)						\
	atomic_compare_exchange_weak_explicit(&(b)->bmtx_lock, o, n,	\
	    memory_order_acq_rel, memory_order_relaxed)
#define bmtx_fetch_or(b,l)						\
	atomic_fetch_or_explicit(&(b)->bmtx_lock, l, memory_order_acq_rel)
#define bmtx_fetch_and(b,l)						\
	atomic_fetch_and_explicit(&(b)->bmtx_lock, l, memory_order_acq_rel)

static inline struct proc *
bmtx_owner(atomic_uintptr_t lock)
{
	return ((struct proc *)(lock & ~BMTX_FLAGS));
}

/*
 * Peek the state of bmtx and return non-zero if the caller should spin or
 * block. We want to spin in case the bmtx owner is running (in this case in
 * another cpu), or if the lock is not "held".
 */
static inline int
bmtx_owner_blocked(atomic_uintptr_t lock)
{
	struct proc *p = bmtx_owner(lock);
	/* XXX if we are preempted here, high chance p might have gone and
	 * we're fucked. This is an "accepted" race in freebsd because of the
	 * way they recycle the td structures, it might not be to us. We could
	 * think of a crit_enter()/crit_leave().
	 */
	if (p)
		return (p->p_stat != SONPROC);

	return (0);		/* No proc, so bmtx is not held */
}

void
bmtx_lock_block(struct bmtx *bmtx, uintptr_t lock)
{
	struct proc *p;

	BST(bmtx_lock_blocks);

	bmtx_inter_lock(bmtx);
	/*
	 * Our caller checks BMTX_WAITERS without the inter lock held for
	 * performance reasons, we must recover from the race
	 */
	if (!bmtx_owner_blocked(bmtx->bmtx_lock)) {
		BST(bmtx_lock_blocks_cancelled);
		bmtx_inter_unlock(bmtx);
		return;
	}
	/*
	 * Set waiter, if something changed, cancel the operation. This will
	 * cause us to re-evaluate bmtx.
	 */
	if (!bmtx_cmp_set(bmtx, &lock,  lock | BMTX_WAITERS)) {
		BST(bmtx_lock_blocks_cancelled);
		bmtx_inter_unlock(bmtx);
		return;
	}
	BST(bmtx_lock_blocks_slept);
	/* We really are destined to sleep */
	p = curproc;
	TAILQ_INSERT_TAIL(&bmtx->bmtx_waiters, p, p_runq);
	p->p_wchan = bmtx;
	p->p_wmesg = bmtx->bmtx_name;
	p->p_slptime = 0;
	p->p_priority = PVM & PRIMASK;
	p->p_stat = SSLEEP;
	mi_switch();
	/* bmtx_inter_unlock(bmtx); SCHED_LOCK is released on mi_switch() */
}

/* Public API */

void
bmtx_init(struct bmtx *bmtx, const char *name)
{
	bmtx->bmtx_recurse = 0;
	bmtx->bmtx_lock = 0;
	bmtx->bmtx_name = name;
	TAILQ_INIT(&bmtx->bmtx_waiters);
}

int
bmtx_held(struct bmtx *bmtx)
{
	return (bmtx_owner(bmtx->bmtx_lock) == curproc);
}

void
bmtx_lock(struct bmtx *bmtx)
{
	uintptr_t lock = 0;
	struct proc *p = curproc;

	/* Try to cheat */
	if (bmtx_cmp_set(bmtx, &lock, (uintptr_t)p)) {
		BST(bmtx_fast_locks);
		return;
	}

	lock = bmtx->bmtx_lock;

	/* Recursion case, try to avoid the atomic call as much as possible */
	if (bmtx_owner(lock) == p) {
		if ((lock & BMTX_RECURSED) == 0)
			bmtx_fetch_or(bmtx, BMTX_RECURSED);
		bmtx->bmtx_recurse++;
		BST(bmtx_recursed_locks);
		return;
	}

	for (;;) {
		/*
		 * If there is no owner, try to get it, but keep the WAITER
		 * flag. At this point it the lock can't be RECURSED, but it
		 * can still be WAITERing.
		 */
		if (bmtx_owner(lock) == NULL) {
			if (bmtx_cmp_set(bmtx, &lock,
			    (uintptr_t)p | (lock & BMTX_WAITERS))) {
				BST(bmtx_spun_locks);
				return;
			}
		} else if (bmtx_owner_blocked(lock))
			bmtx_lock_block(bmtx, lock);
		/* Refresh lock view */
		lock = bmtx->bmtx_lock;
	}
}

void
bmtx_unlock(struct bmtx *bmtx)
{
	struct proc *p = curproc;
	uintptr_t lock = (uintptr_t)p;

	/* Try fast path unlock, uncontested */
	if (bmtx_cmp_set(bmtx, &lock, 0)) {
		BST(bmtx_fast_unlocks);
		return;
	}

	/* Deal with recursion */
	if (bmtx->bmtx_lock & BMTX_RECURSED) {
		if (--bmtx->bmtx_recurse == 0)
			bmtx_fetch_and(bmtx, ~BMTX_RECURSED);
		BST(bmtx_recursed_unlocks);
		return;
	}

	/*
	 * Since the fast path failed, and it's not recursed, the only possible
	 * scenario is a BMTX_WAITER
	 */
	if (__predict_false((bmtx->bmtx_lock & BMTX_WAITERS) == 0))
		panic("%s:%d Unexpected lock word state bmtx=%p lock=%p\n",
		    bmtx, bmtx->bmtx_lock);

	/* So this is a contested mutex, we have to interlock */
	bmtx_inter_lock(bmtx);

	p = TAILQ_FIRST(&bmtx->bmtx_waiters);
	if (p) {
		/* XXX this is HORRIBLE we need better sched api */
		TAILQ_REMOVE(&bmtx->bmtx_waiters, p, p_runq);
		p->p_wchan = NULL;
		p->p_wmesg = NULL;
		p->p_stat = SRUN;
		p->p_cpu = sched_choosecpu(p);
		if (p->p_slptime > 1)
			updatepri(p);
		p->p_slptime = 0;
		setrunqueue(p);
		resched_proc(p, p->p_priority);
	}

	/*
	 * At this point we know that the lock is not BMTX_RECURSE and we are
	 * the ones holding it, so we have to clear the ownership and
	 * recalculate if there are still any BMTX_WAITERS. BMTX_WAITERS CANNOT
	 * be set without interlock, the only other flag would be BMTX_RECURSE
	 * which is also impossible to be set at this point, so we can be sure
	 * that _nothing_ else can change the atomic word, we are free to
	 * atomic_clear it without cmp_set.
	 */
	lock = TAILQ_EMPTY(&bmtx->bmtx_waiters) ? 0 : BMTX_WAITERS;
	bmtx_fetch_and(bmtx, lock);
	bmtx_inter_unlock(bmtx);

	BST(bmtx_blocked_unlocks);
}

int
bmtx_unlock_all(struct bmtx *bmtx)
{
	int count = bmtx->bmtx_recurse;

	bmtx->bmtx_recurse = 0;
	bmtx_fetch_and(bmtx, ~BMTX_RECURSED);
	bmtx_unlock(bmtx);

	return (count + 1);
}

void
bmtx_dump(struct bmtx *bmtx)
{
	atomic_uintptr_t lock = bmtx->bmtx_lock;
	struct proc *p = bmtx_owner(lock);
	int recurse = bmtx->bmtx_recurse;

	printf("bmtx %s (%p): ", bmtx->bmtx_name, bmtx);
	if (p)
		printf("locked by\n  %s, proc=%p, pid=%llu.\n",
		    p->p_comm, p, p->p_pid);
	else
		printf("unlocked\n.");
	printf("  flags=0x%x(%s%s), recurse=%d.\n", lock & BMTX_FLAGS,
	    lock & BMTX_WAITERS ? "BMTX_WAITERS" : "",
	    lock & BMTX_RECURSED ? ",BMTX_RECURSED" : "",
	    recurse);
	if (lock & BMTX_WAITERS) {
		printf("  waiters:\n");
		TAILQ_FOREACH(p, &bmtx->bmtx_waiters, p_runq) {
			printf("    %s, proc=%p, pid=%llu.\n",
			    p->p_comm, p, p->p_pid);
		}
		printf("\n");
	}
}

#include <machine/cpufunc.h>
/* int bmtx_test_n = 100000; */
int bmtx_test_n = 0;
void
bmtx_test(void)
{
	struct bmtx test;
	uint64_t x;
	int n = bmtx_test_n;

	if (bmtx_test_n == 0)
		return;

	bmtx_init(&test, "test bmtx");
	x = rdtsc();
	while (n--) {
		bmtx_lock(&test);
		bmtx_unlock(&test);
	}
	x = rdtsc() - x;
	printf("%s: %d locks/unlocks took %llu cycles, avg=%llu\n",
	    __func__, bmtx_test_n, x, x/bmtx_test_n);
}
