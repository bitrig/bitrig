/*
 * Copyright (c) 2013 Christiano F. Haesbaert <haesbaert@haesbaert.org>
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
#include <sys/ithread.h>
#include <sys/kthread.h>
#include <sys/queue.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>	/* XXX */

//#define ITHREAD_DEBUG
#ifdef ITHREAD_DEBUG
int ithread_debug = 10;
#define DPRINTF(l, x...)	do { if ((l) <= ithread_debug) printf(x); } while (0)
#else
#define DPRINTF(l, x...)
#endif	/* ITHREAD_DEBUG */

TAILQ_HEAD(, intrsource) ithreads = TAILQ_HEAD_INITIALIZER(ithreads);

void
ithread(void *v_is)
{
	struct intrsource *is = v_is;
	struct pic *pic = is->is_pic;
	struct intrhand *ih;
	int irc, stray;

	KASSERT(curproc == is->is_proc);
	sched_peg_curproc(&cpu_info_primary);
	KERNEL_UNLOCK();

	DPRINTF(1, "ithread %u pin %d started\n",
	    curproc->p_pid, is->is_pin);

	for (; ;) {
		stray = 1;

		for (ih = is->is_handlers; ih != NULL; ih = ih->ih_next) {
			if ((ih->ih_flags & IPL_MPSAFE) == 0)
				KERNEL_LOCK();

			is->is_scheduled = 0; /* protected by is->is_maxlevel */

			crit_enter();
			irc = (*ih->ih_fun)(ih->ih_arg);
			crit_leave();

			if ((ih->ih_flags & IPL_MPSAFE) == 0)
				KERNEL_UNLOCK();

			if (intr_shared_edge == 0 && irc > 0) {
				ih->ih_count.ec_count++;
				stray = 0;
				break;
			}
		}

		KASSERT(CRIT_DEPTH == 0);

		if (stray)
			DPRINTF(1, "stray interrupt pin %d ?\n", is->is_pin);

		pic->pic_hwunmask(pic, is->is_pin);

		/*
		 * is->is_scheduled might have been set again (we got another
		 * interrupt), there would be a race here from checking
		 * is_scheduled and calling ithread_sleep(), but ithread sleep
		 * will double check this with a critical section, recovering
		 * from the race.
		 */
		if (!is->is_scheduled) {
			ithread_sleep(is);
			DPRINTF(20, "ithread %u woke up\n", curproc->p_pid);
		}
	}
}


/*
 * This is the hook that gets called in the interrupt frame.
 * XXX intr_shared_edge is not being used, and we're being pessimistic.
 */
void
ithread_run(struct intrsource *is)
{
#ifdef ITHREAD_DEBUG
	struct cpu_info *ci = curcpu();
#endif
	struct proc *p = is->is_proc;

	if (p == NULL) {
		is->is_scheduled = 1;
		DPRINTF(1, "received interrupt pin %d before ithread is ready",
		    is->is_pin);
		return;
	}

	DPRINTF(10, "ithread accepted interrupt pin %d "
	    "(ilevel = %d, maxlevel = %d)\n",
	    is->is_pin, ci->ci_ilevel, is->is_maxlevel);

	SCHED_LOCK();		/* implies crit_enter() ! */
	
	is->is_scheduled = 1;

	switch (p->p_stat) {
	case SRUN:
	case SONPROC:
		break;
	case SSLEEP:
		/* XXX ithread may be blocked on a lock */
		if (p->p_wchan != p)
			goto unlock;
		unsleep(p);
		p->p_stat = SRUN;
		p->p_slptime = 0;
		/*
		 * Setting the thread to runnable is cheaper than a normal
		 * process since the process state can be protected by blocking
		 * interrupts. There is also no need to choose a cpu since we're
		 * pinned. XXX we're not there yet and still rely on normal
		 * SCHED_LOCK crap.
		 */
		setrunqueue(p);
		resched_proc(p, p->p_priority);
		if (kernel_preemption) {
			if (p->p_priority < curproc->p_priority)
				curproc->p_preempt = 1;
		}
		break;
	default:
		SCHED_UNLOCK();
		panic("ithread_handler: unexpected thread state %d\n", p->p_stat);
	}
unlock:
	SCHED_UNLOCK();
}

void
ithread_register(struct intrsource *is)
{
	struct intrsource *is2;

	/* Prevent multiple inclusion */
	TAILQ_FOREACH(is2, &ithreads, entry) {
		if (is2 == is)
			return;
	}

	is->is_run = ithread_run;

	DPRINTF(1, "ithread_register intrsource pin %d\n", is->is_pin);

	TAILQ_INSERT_TAIL(&ithreads, is, entry);
}

void
ithread_deregister(struct intrsource *is)
{
	DPRINTF(1, "ithread_deregister intrsource pin %d\n", is->is_pin);

	is->is_run = NULL;

	TAILQ_REMOVE(&ithreads, is, entry);
}

void
ithread_forkall(void)
{
	struct intrsource *is;
	static int softs;
	char name[MAXCOMLEN+1];

	TAILQ_FOREACH(is, &ithreads, entry) {
		DPRINTF(1, "ithread forking intrsource pin %d\n", is->is_pin);

		if (is->is_pic == &softintr_pic) {
			snprintf(name, sizeof name, "ithread soft %d",
			    softs++);
			if (kthread_create(ithread, is, &is->is_proc, name))
				panic("ithread_forkall");
		} else {
			snprintf(name, sizeof name, "ithread pin %d",
			    is->is_pin);
			if (kthread_create(ithread, is, &is->is_proc, name))
				panic("ithread_forkall");
		}
	}
}

/*
 * Generic painfully slow soft interrupts, this is a temporary implementation to
 * allow us to kill the IPL subsystem and remove all "interrupts" from the
 * system. In the future we'll have real message passing for remote scheduling
 * and one softint per cpu when applicable. These soft threads interlock through
 * SCHED_LOCK, which is totally unacceptable in the future.
 */
struct intrsource *
ithread_softregister(int level, int (*handler)(void *), void *arg, int flags)
{
	struct intrsource *is;
	struct intrhand *ih;

	is = malloc(sizeof(*is), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (is == NULL)
		panic("ithread_softregister");

	is->is_type = IST_LEVEL; /* XXX more like level than EDGE */
	is->is_pic = &softintr_pic;
	is->is_minlevel = IPL_HIGH;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ih == NULL)
		panic("ithread_softregister");

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_level = level;
	ih->ih_flags = flags;
	ih->ih_pin = 0;
	ih->ih_cpu = &cpu_info_primary;

	/* Just prepend it */
	ih->ih_next = is->is_handlers;
	is->is_handlers = ih;

	if (ih->ih_level > is->is_maxlevel)
		is->is_maxlevel = ih->ih_level;
	if (ih->ih_level < is->is_minlevel) /* XXX minlevel will be gone */
		is->is_minlevel = ih->ih_level;

	ithread_register(is);

	return (is);
}

void
ithread_softderegister(struct intrsource *is)
{
	if (!cold)
		panic("ithread_softderegister only works on cold case");

	ithread_deregister(is);
	free(is, M_DEVBUF);
}

/* XXX kern_synch.c, temporary */
#define TABLESIZE	128
#define LOOKUP(x)	(((long)(x) >> 8) & (TABLESIZE - 1))
extern TAILQ_HEAD(slpque,proc) slpque[TABLESIZE];

void
ithread_sleep(struct intrsource *is)
{
	struct proc *p = is->is_proc;

	KASSERT(curproc == p);
	KASSERT(p->p_stat == SONPROC);
	KERNEL_ASSERT_UNLOCKED();

	/*
	 * The check for is_scheduled and actually sleeping must be atomic.
	 */
	SCHED_LOCK();
	if (!is->is_scheduled) {
		p->p_wchan = p;
		p->p_wmesg = "softintr";
		p->p_slptime = 0;
		p->p_priority = PVM & PRIMASK;
		TAILQ_INSERT_TAIL(&slpque[LOOKUP(p)], p, p_runq);
		p->p_stat = SSLEEP;
		mi_switch();
	} else
		SCHED_UNLOCK();
}
