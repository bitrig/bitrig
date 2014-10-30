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

#include <machine/intr.h>

void
crit_enter(void)
{
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
	struct proc *p = curproc;

	if (p->p_crit == 1 && p->p_preempt)
		preempt(NULL);

	if (--p->p_crit == 0)
		crit_unpend();

	KASSERT(p->p_crit >= 0);
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
	crit_unpend();

	return (c);
}

void
crit_unpend(void)
{
	struct cpu_info *ci = curcpu();

	KASSERT(CRIT_DEPTH == 0);

	ci->ci_idepth++;
	intr_unpend();	/* MD bits */
	ci->ci_idepth--;

	KASSERT(CRIT_DEPTH == 0);
}
