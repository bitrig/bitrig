/*
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

#ifndef	_SYS_SOFTINTR_H_
#define	_SYS_SOFTINTR_H_

#ifdef _KERNEL

#include <sys/mutex.h>
#include <sys/ithread.h>

/*
 * Generic software interrupt support for all platforms.
 */

#define SOFTINTR_ESTABLISH_MPSAFE	0x01
#define softintr_establish(i, f, a)					\
	softintr_establish_flags(i, f, a, 0)
#define softintr_establish_mpsafe(i, f, a)				\
	softintr_establish_flags(i, f, a, SOFTINTR_ESTABLISH_MPSAFE)

void	*softintr_establish_flags(int, int (*)(void *), void *, int);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(int);
#define softintr_schedule(x) ithread_run(x)

#endif /* _KERNEL */

#endif	/* _SYS_SOFTINTR_H_ */
