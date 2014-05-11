/*	$OpenBSD: crtend.c,v 1.10 2012/12/05 23:19:57 deraadt Exp $	*/
/*	$NetBSD: crtend.c,v 1.1 1996/09/12 16:59:04 cgd Exp $	*/

#include <sys/types.h>
#include "md_init.h"
#include "extern.h"

/* NULL terminate these lists. */
static init_f __CTOR_LIST__[1]
    __used __attribute__((section(".ctors"))) = { NULL };
static init_f __DTOR_LIST__[1]
    __used __attribute__((section(".dtors"))) = { NULL };
static init_f __init_array[1]
    __used __attribute__((section(".init_array"))) = { NULL };
static init_f __fini_array
    __used __attribute__((section(".fini_array"))) = { NULL };

static const int __EH_FRAME_END__[]
    __used __attribute__((section(".eh_frame"), aligned(4))) = { 0 };

static void * __JCR_END__[]
    __used __attribute__((section(".jcr"), aligned(sizeof(void*)))) = { 0 };

void _do_init_array(void) __dso_hidden __used;
void __do_fini_array(void) __dso_hidden __used;

MD_SECT_CALL_FUNC(".init", __do_init_array);
MD_SECT_CALL_FUNC(".fini", __do_fini_array);

MD_SECTION_EPILOGUE(".init");
MD_SECTION_EPILOGUE(".fini");
