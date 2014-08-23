/*
 * Copyright (c) 2013 Christiano F. Haesbaert <haesbaert@haesbaert.org>
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/softintr.h>

/*
 * Generic painfully slow soft interrupts, this is a temporary implementation to
 * allow us to kill the IPL subsystem and remove all "interrupts" from the
 * system. In the future we'll have real message passing for remote scheduling
 * and one softint per cpu when applicable. These soft threads interlock through
 * SCHED_LOCK, which is totally unacceptable in the future.
 */
void *
softintr_establish_flags(int level, int (*handler)(void *), void *arg,
    int flags)
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
softintr_disestablish(void *is)
{
	if (!cold)
		panic("ithread_softderegister only works on cold case");

	ithread_deregister(is);
	free(is, M_DEVBUF);
}

void
softintr_init(void)
{
}

void	softintr_dispatch(int);
