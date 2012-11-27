/*
 * Copyright (c) 2009 Dale Rahn <drahn@openbsd.org>
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

#include <sys/types.h>
#include <unistd.h>
#include "thread_private.h"

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

#ifdef __ia64__
void	*__tls_get_addr(size_t m, size_t offset) __attribute__((weak));
#else
void	*__tls_get_addr(tls_index *ti) __attribute__((weak));
#endif
#ifdef __i386__
__attribute__ ((weak,__regparm__ (1))) void * 
___tls_get_addr (tls_index *ti __attribute__((__unused__)));
#endif

/*ARGSUSED*/
#ifdef __ia64__
void	*__tls_get_addr(size_t m __attribute__((__unused__)),
    size_t offset __attribute__((__unused__)))
#else
void *__tls_get_addr (tls_index *ti __attribute__((__unused__)))
#endif
{
#define MSG "libc:__tls_get_addr called\n"
	write(2, MSG, sizeof(MSG)-1);
#undef MSG
	return NULL;
}

#ifdef __i386__
/*ARGSUSED*/
__attribute__ ((weak,__regparm__ (1)))
void *___tls_get_addr (tls_index *ti __attribute__((__unused__)))
{
#define MSG "libc:___tls_get_addr called\n"
	write(2, MSG, sizeof(MSG)-1);
#undef MSG
	return NULL;
}
#endif

/*ARGSUSED*/
void *
_rtld_allocate_tls(void *oldtls __attribute__((__unused__)),
    size_t tcbsize __attribute__((__unused__)),
    size_t tcbalign __attribute__((__unused__)))
{
#define MSG "libc:_rtld_allocate_tls called\n"
	write(2, MSG, sizeof(MSG)-1);
#undef MSG
	return NULL;
}

/*ARGSUSED*/
void
_rtld_free_tls(void *tcb __attribute__((__unused__)),
    size_t tcbsize __attribute__((__unused__)),
    size_t tcbalign __attribute__((__unused__)))
{
#define MSG "libc:_rtld_free_tls called\n"
	write(2, MSG, sizeof(MSG)-1);
#undef MSG
}
