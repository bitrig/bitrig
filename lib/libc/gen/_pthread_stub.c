/*
 * Copyright (c) 2013 Dale Rahn drahn@dalerahn.com
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

/* This implements a veneer so that pthreads will not be called directly
 * nor will libc define symbols such that code compiling against only libc
 * will think it has access to libpthread symbols.
 */

#include <pthread.h>
#include <errno.h>

void *_pthread_getspecific(pthread_key_t key);
int _pthread_setspecific(pthread_key_t key, const void *value);
int _pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int _pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

void *pthread_getspecific(pthread_key_t key) __attribute__((weak));
int pthread_setspecific(pthread_key_t key, const void *value) __attribute__((weak));
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) __attribute__((weak));
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) __attribute__((weak));

void *
_pthread_getspecific(pthread_key_t key)
{
	void *(*func)(pthread_key_t key);
	func = pthread_getspecific;
	if (func == NULL)
		return NULL;
	return func(key);
}

int
_pthread_setspecific(pthread_key_t key, const void *value)
{
	int (*func)(pthread_key_t key, const void *value);
	func = pthread_setspecific;
	if (func == NULL)
		return 0;
	return func(key, value);
}


int
_pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int (*func)(pthread_key_t *key, void (*destructor)(void *));
	func = pthread_key_create;
	if (func == NULL)
		return EPERM;
	return func(key, destructor);
}

int
_pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	int (*func)(pthread_once_t *once_control, void (*init_routine)(void));
	func = pthread_once;
	if (func == NULL)
		return 0;
	return func(once_control, init_routine);
}
