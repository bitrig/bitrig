/*-
 * Copyright (c) 2004 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Define stubs for TLS internals so that programs and libraries can
 * link. In non-static binaries these functions will be replaced by
 * functional versions at runtime from ld.so.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
/* Turn on flag to get kernel API */
#define _DYN_LOADER
#include <sys/exec_elf.h>

#if defined(__ia64__) || defined(__amd64__) || defined(__aarch64__)
#define TLS_TCB_ALIGN 16
#elif defined(__powerpc__) || defined(__i386__) || defined(__arm__) || \
    defined(__sparc64__) || defined(__mips__)
#define TLS_TCB_ALIGN sizeof(void *)
#else
#error TLS_TCB_ALIGN undefined for target architecture
#endif

#if defined(__arm__) || defined(__ia64__) || defined(__mips__) || \
    defined(__powerpc__) || defined(__aarch64__)
#define TLS_VARIANT_I
#endif
#if defined(__i386__) || defined(__amd64__) || defined(__sparc64__)
#define TLS_VARIANT_II
#endif


#if !defined(__PIC__) || defined(__PIE__)

/*
 * Static vars usable after bootstrapping.
 */
static void *_lt_malloc_pool = 0;
static long *_lt_malloc_free = 0;

#define LT_MALLOC_ALIGN 8
#define _lt_round_page(x)       (((x) + (__LDPGSZ - 1)) & ~(__LDPGSZ - 1))

void _lt_free(void *p);
void * __lt_malloc(size_t need);

/*
 * The following malloc/free code is a very simplified implementation
 * of a malloc function.
 */
void *
_lt_malloc(size_t need)
{
	long *p, *t, *n, have;

	need = (need + 2*LT_MALLOC_ALIGN - 1) & ~(LT_MALLOC_ALIGN - 1);

	if ((t = _lt_malloc_free) != 0) {	/* Try free list first */
		n = (long *)&_lt_malloc_free;
		while (t && t[-1] < need) {
			n = t;
			t = (long *)*t;
		}
		if (t) {
			*n = *t;
			memset(t, 0, t[-1] - LT_MALLOC_ALIGN);
			return((void *)t);
		}
	}
	have = _lt_round_page((long)_lt_malloc_pool) - (long)_lt_malloc_pool;
	if (need > have) {
		if (have >= 8 + LT_MALLOC_ALIGN) {
			p = _lt_malloc_pool;
			p = (void *) ((long)p + LT_MALLOC_ALIGN);
			p[-1] = have;
			_lt_free((void *)p);	/* move to freelist */
		}
		_lt_malloc_pool = (void *)mmap((void *)0,
		    _lt_round_page(need), PROT_READ|PROT_WRITE,
		    MAP_ANON|MAP_PRIVATE, -1, 0);
		if (_lt_malloc_pool == MAP_FAILED) {
			return 0;
		}
	}
	p = _lt_malloc_pool;
	_lt_malloc_pool += need;
	memset(p, 0, need);
	p = (void *) ((long)p + LT_MALLOC_ALIGN);
	p[-1] = need;
	return (p);
}

void
_lt_free(void *p)
{
        long *t = (long *)p;

        *t = (long)_lt_malloc_free;
        _lt_malloc_free = p;
}


#define round(size, align) \
        (((size) + (align) - 1) & ~((align) - 1))

static ssize_t tls_static_space;
static ssize_t tls_init_size;
static void *tls_init;

#ifdef TLS_VARIANT_I

#define TLS_TCB_SIZE    (2 * sizeof(void *))

/*
 * Free Static TLS using the Variant I method.
 */
void
__libc_free_tls(void *tcb, size_t tcbsize, size_t tcbalign __unused)
{
	Elf_Addr *dtv;
	Elf_Addr **tls;

	tls = (Elf_Addr **)((Elf_Addr)tcb + tcbsize - TLS_TCB_SIZE);
	dtv = tls[0];
	_lt_free(dtv);
	_lt_free(tcb);
}

/*
 * Allocate Static TLS using the Variant I method.
 */
void *
__libc_allocate_tls(void *oldtcb, size_t tcbsize, size_t tcbalign __unused)
{
	Elf_Addr *dtv;
	Elf_Addr **tls;
	char *tcb;

	if (oldtcb != NULL && tcbsize == TLS_TCB_SIZE)
		return (oldtcb);

	tcb = _lt_malloc(tls_static_space + tcbsize - TLS_TCB_SIZE);
	tls = (Elf_Addr **)(tcb + tcbsize - TLS_TCB_SIZE);

	if (oldtcb != NULL) {
		memcpy(tls, oldtcb, tls_static_space);
		_lt_free(oldtcb);

		/* Adjust the DTV. */
		dtv = tls[0];
		dtv[2] = (Elf_Addr)tls + TLS_TCB_SIZE;
	} else {
		dtv = _lt_malloc(3 * sizeof(Elf_Addr));
		tls[0] = dtv;
		dtv[0] = 1;
		dtv[1] = 1;
		dtv[2] = (Elf_Addr)tls + TLS_TCB_SIZE;

		if (tls_init_size > 0)
			memcpy((void*)dtv[2], tls_init, tls_init_size);
		if (tls_static_space > tls_init_size)
			memset((void*)(dtv[2] + tls_init_size), 0,
			    tls_static_space - tls_init_size);
	}

	return(tcb);
}

#endif //  TLS_VARIANT_I

#ifdef TLS_VARIANT_II

#define TLS_TCB_SIZE    (3 * sizeof(Elf_Addr))

/*
 * Free Static TLS using the Variant II method.
 */
void
__libc_free_tls(void *tcb, size_t tcbsize __unused, size_t tcbalign)
{
	size_t size;
	Elf_Addr* dtv;
	Elf_Addr tlsstart, tlsend;

	/*
	 * Figure out the size of the initial TLS block so that we can
	 * find stuff which ___tls_get_addr() allocated dynamically.
	 */
	size = round(tls_static_space, tcbalign);

	dtv = ((Elf_Addr**)tcb)[1];
	tlsend = (Elf_Addr) tcb;
	tlsstart = tlsend - size;
	_lt_free((void*) tlsstart);
	_lt_free(dtv);
}

/*
 * Allocate Static TLS using the Variant II method.
 */
void *
__libc_allocate_tls(void *oldtls, size_t tcbsize, size_t tcbalign)
{
	size_t size;
	char *tls;
	Elf_Addr *dtv;
	Elf_Addr segbase, oldsegbase;

	size = round(tls_static_space, tcbalign);

	if (tcbsize < 2 * sizeof(Elf_Addr))
		tcbsize = 2 * sizeof(Elf_Addr);
	tls = _lt_malloc(size + tcbsize);
	dtv = _lt_malloc(3 * sizeof(Elf_Addr));

	segbase = (Elf_Addr)(tls + size);
	((Elf_Addr*)segbase)[0] = segbase;
	((Elf_Addr*)segbase)[1] = (Elf_Addr) dtv;

	dtv[0] = 1;
	dtv[1] = 1;
	dtv[2] = segbase - tls_static_space;

	if (oldtls) {
		/*
		 * Copy the static TLS block over whole.
		 */
		oldsegbase = (Elf_Addr) oldtls;
		memcpy((void *)(segbase - tls_static_space),
		    (const void *)(oldsegbase - tls_static_space),
		    tls_static_space);

		/*
		 * We assume that this block was the one we created with
		 * allocate_initial_tls().
		 */
		__libc_free_tls(oldtls, 2*sizeof(Elf_Addr), sizeof(Elf_Addr));
	} else {
		memcpy((void *)(segbase - tls_static_space),
		    tls_init, tls_init_size);
		memset((void *)(segbase - tls_static_space + tls_init_size),
		    0, tls_static_space - tls_init_size);
	}

	return (void*) segbase;
}

#endif /* TLS_VARIANT_II */

#endif /* !defined(__PIC__) || defined(__PIE__) */

void __set_tcb(void*);

void
__init_tcb(__unused void **envp)
{
#if !defined(__PIC__) || defined(__PIE__)
	Elf_Addr *sp;
	AuxInfo *aux, *auxp;
	Elf_Phdr *phdr;
	size_t phent, phnum;
	int i;
	void *tls;

	sp = (Elf_Addr *) envp;
	while (*sp++ != 0)
		;
	aux = (AuxInfo *) sp;
	phdr = 0;
	phent = phnum = 0;
	for (auxp = aux; auxp->au_id != AUX_null; auxp++) {
		switch (auxp->au_id) {
		case AUX_phdr:
			phdr = (Elf_Phdr *)auxp->au_v;
			break;

		case AUX_phent:
			phent = auxp->au_v;
			break;

		case AUX_phnum:
			phnum = auxp->au_v;
			break;
		}
	}
	if (phdr == 0 || phent != sizeof(Elf_Phdr) || phnum == 0) {
		// try tls symbols
		extern char *__tdata_start, *__tdata_end;
		tls_init = (void*)&__tdata_start;
		tls_init_size = (char*)&__tdata_end - (char*)&__tdata_start;
		tls_static_space = round(tls_init_size, sizeof(long));
	} else {
		for (i = 0; (unsigned) i < phnum; i++) {
			if (phdr[i].p_type == PT_TLS) {
				tls_static_space = round(phdr[i].p_memsz,
				    phdr[i].p_align);
				tls_init_size = phdr[i].p_filesz;
				tls_init = (void*) phdr[i].p_vaddr;
			}
		}
	}

#ifdef TLS_VARIANT_I
	/*
	 * tls_static_space should include space for TLS structure
	 */
	tls_static_space += TLS_TCB_SIZE;
#endif

	tls = __libc_allocate_tls(NULL, TLS_TCB_SIZE, TLS_TCB_ALIGN);

	__set_tcb(tls);
#endif /* !defined(__PIC__) || defined(__PIE__) */
}
