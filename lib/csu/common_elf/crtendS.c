/*	$OpenBSD: crtendS.c,v 1.8 2012/12/05 23:19:57 deraadt Exp $	*/
/*	$NetBSD: crtend.c,v 1.1 1997/04/16 19:38:24 thorpej Exp $	*/

#include <sys/types.h>
#include "md_init.h"
#include "extern.h"

/* NULL terminate these lists. */
static init_f __CTOR_LIST__[1]
    __used __attribute__((section(".ctors"))) = { (void *)0 };
static init_f __DTOR_LIST__[1]
    __used __attribute__((section(".dtors"))) = { (void *)0 };
static init_f _init_array[1]
    __used __attribute__((section(".init_array"))) = { (void *)0 };
static init_f _fini_array[1]
    __used __attribute__((section(".fini_array"))) = { (void *)0 };

static void * __JCR_END__[]
    __used __attribute__((section(".jcr"), aligned(sizeof(void*)))) = { 0 };

void _do_init_array(void) __dso_hidden __used;
void _do_fini_array(void) __dso_hidden __used;

MD_SECT_CALL_FUNC(".init", _do_init_array);
MD_SECT_CALL_FUNC(".fini", _do_fini_array);

MD_SECTION_EPILOGUE(".init");
MD_SECTION_EPILOGUE(".fini");
