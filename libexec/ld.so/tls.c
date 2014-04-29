/* $OpenBSD$ */
/*-
 * Copyright 1996, 1997, 1998, 1999, 2000 John D. Polstra.
 * Copyright 2003 Alexander Kabaev <kan@FreeBSD.ORG>.
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
 *
 * $FreeBSD: src/libexec/rtld-elf/rtld.c,v 1.134 2009/04/03 19:17:23 kib Exp $
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/exec.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <nlist.h>
#include <string.h>
#include <link.h>
#include <dlfcn.h>

#include "archdep.h"
#include "resolve.h"
#include "util.h"
#include "tls.h"

/*
 * Globals to control TLS allocation.
 */
size_t _dl_tls_static_space;        /* Static TLS space allocated */
int _dl_tls_dtv_generation = 1;     /* Used to detect when dtv size changes  */
int _dl_tls_max_index = 1;          /* Largest module index allocated */
int _dl_tls_free_idx = 0;
int _dl_tls_first_done = 0;

void
_dl_allocate_tls_offset(elf_object_t *object)
{
	if (object->tls_done)
		return;

	if (object->tls_msize == 0) {
		object->tls_done = 1;
		return;
	}

#if TLS_VARIANT == 1
	/* round up to the required alignment, then allocate the space */
	object->tls_offset = ELF_ROUND(_dl_tls_free_idx, object->tls_align);
	_dl_tls_free_idx += object->tls_msize;
#elif TLS_VARIANT == 2
	/*
	 * allocate the space, then round up to the alignment
	 * (these are negative offsets, so rounding up really rounds the
	 * address down)
	 */
	_dl_tls_free_idx = ELF_ROUND(_dl_tls_free_idx + object->tls_msize,
	    object->tls_align);
	object->tls_offset = _dl_tls_free_idx;
#else
	#error Invalid TLS_VARIANT
#endif
	DL_DEB(("allocating object %s to offset %d msize %d\n",
	    object->load_name, object->tls_offset, object->tls_msize));

	object->tls_done = 1;
}

/*
 * allocate TLS for some archs (Variant II)
 * i386, amd64, sparc?, sparc64, mips, more?
 */
void *
_dl_allocate_tls(char *oldtls, elf_object_t *objhead, size_t tcbsize,
    size_t tcbalign)
{
	size_t size;
	char *allocp, *p;
	Elf_Addr *dtv;
	Elf_Addr segbase; /* same value as tcb but no pointer math */
	struct thread_control_block *tcb;

	size = ELF_ROUND(_dl_tls_static_space, tcbalign);
	p = _dl_malloc(size + tcbsize);
	dtv = _dl_malloc((_dl_tls_max_index+2) * sizeof(Elf_Addr));

#if TLS_VARIANT == 1
	tcb = (struct thread_control_block *)p;
	segbase = (Elf_Addr)p;
	p += sizeof(struct thread_control_block);
#elif TLS_VARIANT == 2
	tcb = (struct thread_control_block *)(p+size);
	segbase = (Elf_Addr)tcb;
	tcb->__tcb_self = tcb;
#endif
	allocp = p;
	tcb->tcb_dtv = dtv;

	dtv[0] = _dl_tls_dtv_generation;
	dtv[1] = _dl_tls_max_index;

	DL_DEB(("allocate_tls %p -> %p dtv %p -> %p gen %u max %u size %u tcbsize %u\n",
	    oldtls, segbase, oldtls ? ((Elf_Addr**)oldtls)[1] : NULL, dtv,
	    _dl_tls_dtv_generation, _dl_tls_max_index, size, tcbsize));
	if (oldtls != NULL) {
		Elf_Addr *olddtv;
		Elf_Addr oldsegbase;
		int olddtvsize, i;

		/*
		 * Copy the static TLS block over whole.
		 */
		oldsegbase = (Elf_Addr) oldtls;

#if TLS_VARIANT == 1
		DL_DEB(("allocate_tls copying %p to %p\n",
			(void *)(oldsegbase), (void *)(p)));

		_dl_bcopy((void *)(oldsegbase +
		    sizeof(struct thread_control_block)), (void *)(p),
		    _dl_tls_static_space);
#elif TLS_VARIANT == 2
		DL_DEB(("allocate_tls copying %p to %p\n",
			(void *)(oldsegbase - _dl_tls_static_space),
			    (void *)(segbase - _dl_tls_static_space)));

		_dl_bcopy((void *)(oldsegbase - _dl_tls_static_space),
		    (void *)(segbase - _dl_tls_static_space),
		    _dl_tls_static_space);
#endif

		/*
		 * If any dynamic TLS blocks have been created by
		 * tls_get_addr(), move them over.  Adjust pointers
		 * into the static TLS block
		 */
		olddtv = ((Elf_Addr**)oldsegbase)[1];
		olddtvsize = olddtv[1] - 1;
		DL_DEB(("\tscanning %u entries from %p\n", olddtvsize, olddtv));
		for (i = 0; i < olddtvsize; i++) {
			DL_DEB(("\t%p", olddtv[i+2]));
			Elf_Addr seg_start, seg_end;
#if TLS_VARIANT == 1
			seg_start = oldsegbase;
			seg_end = oldsegbase + size;
#elif TLS_VARIANT == 2
			seg_start = oldsegbase - size;
			seg_end = oldsegbase;
#endif
			if (olddtv[i+2] < seg_start ||
			    olddtv[i+2] >= seg_end) {
				dtv[i+2] = olddtv[i+2];
				olddtv[i+2] = 0;
			} else {
				dtv[i+2] = olddtv[i+2] - oldsegbase + segbase;
				DL_DEB((" -> %p", dtv[i+2]));
			}
			DL_DEB(("\n"));
		}

		/*
		 * We assume that this block was the one we created with
		 * allocate_initial_tls().
		 */
		 _dl_free_tls(oldtls, 2*sizeof(Elf_Addr), sizeof(Elf_Addr));
	} else {
		elf_object_t *obj;

		for (obj = objhead; obj; obj = obj->next) {
			if (obj->tls_msize) {
#if TLS_VARIANT == 1
				Elf_Addr addr = (Elf_Addr)(p + obj->tls_offset);
#elif TLS_VARIANT == 2
				Elf_Addr addr = segbase - obj->tls_offset;
#endif

				_dl_memset((void *)(addr + obj->tls_fsize), 0,
				    obj->tls_msize - obj->tls_fsize);
				if (obj->tls_static_data) 
					_dl_bcopy(obj->tls_static_data,
					    (void *)addr, obj->tls_fsize);
				DL_DEB(("\t%s has index %u addr %p msize %u fsize %u\n",
					obj->load_name, obj->tls_offset,
					(void *)addr, obj->tls_msize,
					obj->tls_fsize));
				dtv[obj->tls_index + 1] = addr;
			}
		}
	}

	DL_DEB(("returning segbase %p val %p %p %p dtv %p\n", segbase,
	    ((Elf_Addr*)segbase)[0], &((Elf_Addr*)segbase)[1], ((Elf_Addr*)segbase)[1], dtv));
	return (void*) segbase;
}

void
_dl_free_tls(void *tls, size_t tcbsize, size_t tcbalign)
{
	size_t size;
	Elf_Addr* dtv;
	int dtvsize, i;
	Elf_Addr tlsstart, tlsend;

	/*
	* Figure out the size of the initial TLS block so that we can
	* find stuff which ___tls_get_addr() allocated dynamically.
	*/
	size = ELF_ROUND(_dl_tls_static_space, tcbalign);

	dtv = ((Elf_Addr**)tls)[1];
	DL_DEB(("free_tls %p seg %p dtv %p\n",
	    ((Elf_Addr**)tls)[0], &((Elf_Addr**)tls)[1], ((Elf_Addr**)tls)[1]));
	dtvsize = dtv[1] - 1;
	tlsend = (Elf_Addr) tls;
	tlsstart = tlsend - size;
	DL_DEB(("free_tls %p dtv %p gen %u max %u size %u tcbsize %u\n",
	    tls, dtv, dtv[0], dtv[1], size, tcbsize));
	for (i = 0; i < dtvsize; i++) {
		if (dtv[i+2] && (dtv[i+2] < tlsstart || dtv[i+2] > tlsend)) {
			_dl_free((void*) dtv[i+2]);
		}
	}

	_dl_free((void*) tlsstart);
	_dl_free((void*) dtv);
}

/*
 * Allocate TLS block for module with given index.
 */
void *
_dl_allocate_module_tls(int index)
{
	elf_object_t *obj;
	char* p;

	for (obj = _dl_objects; obj; obj = obj->next) {
		if (obj->tls_index == index)
			break;
	}
	if (!obj) {
		_dl_printf("Can't find module with TLS index %d", index);
		_dl_exit(20);
	}

	p = _dl_malloc(obj->tls_msize);
	_dl_bcopy(obj->tls_static_data, p, obj->tls_fsize);
	_dl_memset(p + obj->tls_fsize, 0, obj->tls_msize - obj->tls_fsize);

	return p;
}

void *
_rtld_allocate_tls(void *oldtls, size_t tcbsize, size_t tcbalign)
{
	sigset_t savedmask;
	void *ret;

	_dl_thread_bind_lock(0, &savedmask);
	ret = _dl_allocate_tls(oldtls, _dl_objects, tcbsize, tcbalign);
	_dl_thread_bind_lock(1, &savedmask);
	return (ret);
}

void
_rtld_free_tls(void *tcb, size_t tcbsize, size_t tcbalign)
{
	sigset_t savedmask;

	_dl_thread_bind_lock(0, &savedmask);
	_dl_free_tls(tcb, tcbsize, tcbalign);
	_dl_thread_bind_lock(1, &savedmask);
}

/*
 * Common code for MD __tls_get_addr().
 */
void *
_dl_tls_get_addr_common(Elf_Addr** dtvp, int index, size_t offset)
{
	sigset_t savedmask;
	Elf_Addr* dtv = *dtvp;

	DL_DEB(("tls_get_addr_common %p %d %lu\n", dtv, index, offset));

	/* Check dtv generation in case new modules have arrived */
	if (dtv[0] != _dl_tls_dtv_generation) {
		Elf_Addr* newdtv;
		int to_copy;

		_dl_thread_bind_lock(0, &savedmask);
		newdtv = _dl_malloc((_dl_tls_max_index + 1) * sizeof(Elf_Addr));
		to_copy = dtv[1];
		if (to_copy > _dl_tls_max_index)
			to_copy = _dl_tls_max_index;
		_dl_bcopy(&dtv[2], &newdtv[2], (to_copy-1) * sizeof(Elf_Addr));
		newdtv[0] = _dl_tls_dtv_generation;
		newdtv[1] = _dl_tls_max_index;
		*dtvp = newdtv;
		_dl_thread_bind_lock(1, &savedmask);
		_dl_free(dtv);
		dtv = newdtv;
	}

	/* Dynamically allocate module TLS if necessary */
	if (!dtv[index + 1]) {
		/* Signal safe, wlock will block out signals. */
		_dl_thread_bind_lock(0, &savedmask);
		if (!dtv[index + 1])
			dtv[index + 1] = (Elf_Addr)_dl_allocate_module_tls(index);
		_dl_thread_bind_lock(1, &savedmask);
	}
	return (void*) (dtv[index + 1] + offset);
}
