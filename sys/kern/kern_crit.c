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

#include <machine/cpufunc.h>

void	crit_rundeferred(void);

//#define CRITCOUNTERS
#ifdef CRITCOUNTERS
uint64_t defclock, defother;
#endif

/* XXX we need to make sure we have a process context soon, for now just keep
 * this counter so that we can hunt it. */
u_int crit_escaped;

void
crit_enter(void)
{
	if (curproc == NULL) {
		crit_escaped++;
		return;
	}
	curproc->p_crit++;
}

/*
 * The pair for crit_leave_all()
 */
void
crit_reenter(int c)
{
	curproc->p_crit += c;
}

void
crit_leave(void)
{
	if (curproc == NULL) {
		crit_escaped++;
		return;
	}
	if (--curproc->p_crit == 0)
		crit_rundeferred();

	KASSERT(curproc->p_crit >= 0);
}

/*
 * If you don't know when to use this, don't use it.
 * This is intended for paths where it is totally safe to lose atomicity, be it
 * sleep or anything else. The critical level must be saved on the stack and
 * restored with crit_reenter().
 */
int
crit_leave_all(void)
{
	struct proc *p = curproc;
	int c = p->p_crit;

	p->p_crit = 0;
	crit_rundeferred();

	return (c);
}

/*
 * Must always run with interrupts disabled, ci_ipending is protect by blocking
 * interrupts.
 */
#define IPENDING_ISSET(ci, slot) (ci->ci_ipending & (1ull << slot))
#define IPENDING_CLR(ci, slot) (ci->ci_ipending &= ~(1ull << slot))
#define IPENDING_NEXT(ci) (flsq(ci->ci_ipending) - 1)
void
crit_rundeferred(void)
{
	struct cpu_info *ci = curcpu();
	int i;
	long rf;

	KASSERT(CRIT_DEPTH == 0);

	rf = read_rflags(); /* XXX or psl ? */
	disable_intr();

	ci->ci_idepth++;
	while (ci->ci_ipending) {
		/* XXX can be optimized by having a btrq based flsq */
		i = IPENDING_NEXT(ci);
		IPENDING_CLR(ci, i);
		enable_intr();
		ci->ci_isources[i]->is_run(ci->ci_isources[i]);
		disable_intr();
	}
	ci->ci_idepth--;
	write_rflags(rf);
}
