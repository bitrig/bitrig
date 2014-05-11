/*	$OpenBSD: crtbegin.c,v 1.17 2013/12/28 18:38:42 kettenis Exp $	*/
/*	$NetBSD: crtbegin.c,v 1.1 1996/09/12 16:59:03 cgd Exp $	*/

/*
 * Copyright (c) 1993 Paul Kranenburg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Run-time module for GNU C++ compiled shared libraries.
 *
 * The linker constructs the following arrays of pointers to global
 * constructors and destructors. The first element contains the
 * number of pointers in each.
 * The tables are also null-terminated.
 */
#include <stdlib.h>

#include "md_init.h"
#include "os-note-elf.h"
#include "extern.h"

struct dwarf2_eh_object {
	void *space[8];
};

void __register_frame_info(const void *, struct dwarf2_eh_object *)
    __attribute__((weak));

void __register_frame_info(const void *begin, struct dwarf2_eh_object *ob)
{
}

static const char __EH_FRAME_BEGIN__[]
    __attribute__((section(".eh_frame"), aligned(4))) = { };

/*
 * java class registration hooks
 */

static void *__JCR_LIST__[] __dso_hidden
    __attribute__((section(".jcr"), aligned(sizeof(void*)))) = { };

extern void _Jv_RegisterClasses (void *)
    __attribute__((weak));

/*
 * Include support for the __cxa_atexit/__cxa_finalize C++ abi for
 * gcc > 2.x. __dso_handle is NULL in the main program and a unique
 * value for each C++ shared library. For more info on this API, see:
 *
 *     http://www.codesourcery.com/cxx-abi/abi.html#dso-dtor
 */

void *__dso_handle __dso_hidden = NULL;

long __guard_local __dso_hidden __attribute__((section(".openbsd.randomdata")));

extern int __cxa_atexit(void (*)(void *), void *, void *) __attribute__((weak));

int
atexit(void (*fn)(void))
{
	return (__cxa_atexit((void (*)(void *))fn, NULL, NULL));
}

static init_f __CTOR_LIST__[] __used
    __attribute__((section(".ctors"), aligned(sizeof(init_f)))) = { };
static init_f __DTOR_LIST__[] __used
    __attribute__((section(".dtors"), aligned(sizeof(init_f)))) = { };
static init_f __init_array[] __used
    __attribute__((section(".init_array"), aligned(sizeof(init_f)))) = { };
static init_f __fini_array[] __used
    __attribute__((section(".fini_array"), aligned(sizeof(init_f)))) = { };

static void	__dtors(void) __used;
static void	__ctors(void) __used;

void
__ctors(void)
{
	const init_f *list = __CTOR_LIST__;
	int i;

	for (i = 0; list[i] != NULL; i++)
		;

	while (i-- > 0)
		(**list[i])();
}

void
__dtors(void)
{
	const init_f *p = __DTOR_LIST__;

	while (*p) {
		(**p++)();
	}
}

void __init(void) __dso_hidden;
void __fini(void) __dso_hidden;
void __do_init(void) __dso_hidden __used;
void __do_fini(void) __dso_hidden __used;
void __do_init_array(void) __dso_hidden __used;
void __do_fini_array(void) __dso_hidden __used;

MD_SECTION_PROLOGUE(".init", __init);
MD_SECTION_PROLOGUE(".fini", __fini);

MD_SECT_CALL_FUNC(".init", __do_init);
MD_SECT_CALL_FUNC(".fini", __do_fini);

void
__do_init(void)
{
	static int initialized = 0;
	static struct dwarf2_eh_object object;

	/*
	 * Call global constructors.
	 * Arrange to call global destructors at exit.
	 */
	if (!initialized) {
		initialized = 1;

		__register_frame_info(__EH_FRAME_BEGIN__, &object);

		if (__JCR_LIST__[0] && _Jv_RegisterClasses)
			_Jv_RegisterClasses(__JCR_LIST__);

		__ctors();

		atexit(__fini);
	}
}

void
__do_fini(void)
{
	static int finalized = 0;

	if (!finalized) {
		finalized = 1;
		/*
		 * Call global destructors.
		 */
		__dtors();
	}
}

void
__do_init_array(void)
{
	const init_f *list = __init_array;
	int i;

	for (i = 0; list[i] != NULL; i++)
		;

	while (i-- > 0)
		(**list[i])();
}

void
__do_fini_array(void)
{
	const init_f *p = __fini_array;

	while (*p) {
		(**p++)();
	}
}

