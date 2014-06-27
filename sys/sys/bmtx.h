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

#ifndef _SYS_BMTX_H_
#define _SYS_BMTX_H_

struct bmtx {
	atomic_uintptr_t	bmtx_lock;
	int			bmtx_recurse;
	TAILQ_HEAD(,proc)	bmtx_waiters;
};

/* Flags abusing the 2 lower bits of bmtx_lock */
#define BMTX_WAITERS	0x1
#define BMTX_RECURSED	0x2
#define BMTX_FLAGS	(BMTX_WAITERS|BMTX_RECURSED)

void	bmtx_init(struct bmtx *);
int	bmtx_held(struct bmtx *);
void	bmtx_lock(struct bmtx *);
void	bmtx_unlock(struct bmtx *);
int	bmtx_unlock_all(struct bmtx *);
void	bmtx_test(void);

#endif /* _SYS_BMTX_H_ */
