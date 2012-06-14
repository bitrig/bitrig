/*	$OpenBSD: uvm_km.c,v 1.107 2012/03/09 13:01:29 ariane Exp $	*/
/*	$NetBSD: uvm_km.c,v 1.42 2001/01/14 02:10:01 thorpej Exp $	*/

/* 
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.  
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and 
 *      its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_kern.c   8.3 (Berkeley) 1/12/94
 * from: Id: uvm_km.c,v 1.1.2.14 1998/02/06 05:19:27 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * uvm_km.c: handle kernel memory allocation and management
 */

/*
 * overview of kernel memory management:
 *
 * the kernel virtual address space is mapped by "kernel_map."   kernel_map
 * starts at VM_MIN_KERNEL_ADDRESS and goes to VM_MAX_KERNEL_ADDRESS.
 * note that VM_MIN_KERNEL_ADDRESS is equal to vm_map_min(kernel_map).
 *
 * the kernel_map has several "submaps."   submaps can only appear in 
 * the kernel_map (user processes can't use them).   submaps "take over"
 * the management of a sub-range of the kernel's address space.  submaps
 * are typically allocated at boot time and are never released.   kernel
 * virtual address space that is mapped by a submap is locked by the 
 * submap's lock -- not the kernel_map's lock.
 *
 * thus, the useful feature of submaps is that they allow us to break
 * up the locking and protection of the kernel address space into smaller
 * chunks.
 *
 * The VM system has several standard kernel submaps:
 *   kmem_map: Contains only wired kernel memory for malloc(9).
 *	       Note: All access to this map must be protected by splvm as
 *	       calls to malloc(9) are allowed in interrupt handlers.
 *   exec_map: Memory to hold arguments to system calls are allocated from
 *	       this map.
 *	       XXX: This is primeraly used to artificially limit the number
 *	       of concurrent processes doing an exec.
 *   phys_map: Buffers for vmapbuf (physio) are allocated from this map.
 *
 * the kernel allocates its private memory out of special uvm_objects whose
 * reference count is set to UVM_OBJ_KERN (thus indicating that the objects
 * are "special" and never die).   all kernel objects should be thought of
 * as large, fixed-sized, sparsely populated uvm_objects.   each kernel 
 * object is equal to the size of kernel virtual address space (i.e. the
 * value "VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS").
 *
 * most kernel private memory lives in kernel_object.   the only exception
 * to this is for memory that belongs to submaps that must be protected
 * by splvm(). each of these submaps manages their own pages.
 *
 * note that just because a kernel object spans the entire kernel virtual
 * address space doesn't mean that it has to be mapped into the entire space.
 * large chunks of a kernel object's space go unused either because 
 * that area of kernel VM is unmapped, or there is some other type of 
 * object mapped into that range (e.g. a vnode).    for submap's kernel
 * objects, the only part of the object that can ever be populated is the
 * offsets that are managed by the submap.
 *
 * note that the "offset" in a kernel object is always the kernel virtual
 * address minus the VM_MIN_KERNEL_ADDRESS (aka vm_map_min(kernel_map)).
 * example:
 *   suppose VM_MIN_KERNEL_ADDRESS is 0xf8000000 and the kernel does a
 *   uvm_km_alloc(kernel_map, PAGE_SIZE) [allocate 1 wired down page in the
 *   kernel map].    if uvm_km_alloc returns virtual address 0xf8235000,
 *   then that means that the page at offset 0x235000 in kernel_object is
 *   mapped at 0xf8235000.   
 *
 * kernel objects have one other special property: when the kernel virtual
 * memory mapping them is unmapped, the backing memory in the object is
 * freed right away.   this is done with the uvm_km_pgremove() function.
 * this has to be done because there is no backing store for kernel pages
 * and no need to save them after they are no longer referenced.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <uvm/uvm.h>

/*
 * global data structures
 */

struct vm_map *kernel_map = NULL;

/* Unconstraint range. */
struct uvm_constraint_range	no_constraint = { 0x0, (paddr_t)-1 };

/*
 * local data structues
 */

static struct vm_map		kernel_map_store;

/*
 * uvm_km_init: init kernel maps and objects to reflect reality (i.e.
 * KVM already allocated for text, data, bss, and static data structures).
 *
 * => KVM is defined by VM_MIN_KERNEL_ADDRESS/VM_MAX_KERNEL_ADDRESS.
 *    we assume that [min -> start] has already been allocated and that
 *    "end" is the end.
 */

void
uvm_km_init(vaddr_t start, vaddr_t end)
{
	vaddr_t base = VM_MIN_KERNEL_ADDRESS;

	/*
	 * next, init kernel memory objects.
	 */

	/* kernel_object: for pageable anonymous kernel memory */
	uao_init();
	uvm.kernel_object = uao_create(VM_MAX_KERNEL_ADDRESS -
				 VM_MIN_KERNEL_ADDRESS, UAO_FLAG_KERNOBJ);

	/*
	 * init the map and reserve already allocated kernel space 
	 * before installing.
	 */

	uvm_map_setup(&kernel_map_store, base, end,
#ifdef KVA_GUARDPAGES
	    VM_MAP_PAGEABLE | VM_MAP_GUARDPAGES
#else
	    VM_MAP_PAGEABLE
#endif
	    );
	kernel_map_store.pmap = pmap_kernel();
	if (base != start && uvm_map(&kernel_map_store, &base, start - base,
	    NULL, UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL,
	    UVM_INH_NONE, UVM_ADV_RANDOM,UVM_FLAG_FIXED)) != 0)
		panic("uvm_km_init: could not reserve space for kernel");
	
	/*
	 * install!
	 */

	kernel_map = &kernel_map_store;
}

/*
 * uvm_km_suballoc: allocate a submap in the kernel map.   once a submap
 * is allocated all references to that area of VM must go through it.  this
 * allows the locking of VAs in kernel_map to be broken up into regions.
 *
 * => if `fixed' is true, *min specifies where the region described
 *      by the submap must start
 * => if submap is non NULL we use that as the submap, otherwise we
 *	alloc a new map
 */
struct vm_map *
uvm_km_suballoc(struct vm_map *map, vaddr_t *min, vaddr_t *max, vsize_t size,
    int flags, boolean_t fixed, struct vm_map *submap)
{
	int mapflags = UVM_FLAG_NOMERGE | (fixed ? UVM_FLAG_FIXED : 0);

	size = round_page(size);	/* round up to pagesize */

	/*
	 * first allocate a blank spot in the parent map
	 */

	if (uvm_map(map, min, size, NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
	    UVM_ADV_RANDOM, mapflags)) != 0) {
	       panic("uvm_km_suballoc: unable to allocate space in parent map");
	}

	/*
	 * set VM bounds (min is filled in by uvm_map)
	 */

	*max = *min + size;

	/*
	 * add references to pmap and create or init the submap
	 */

	pmap_reference(vm_map_pmap(map));
	if (submap == NULL) {
		submap = uvm_map_create(vm_map_pmap(map), *min, *max, flags);
		if (submap == NULL)
			panic("uvm_km_suballoc: unable to create submap");
	} else {
		uvm_map_setup(submap, *min, *max, flags);
		submap->pmap = vm_map_pmap(map);
	}

	/*
	 * now let uvm_map_submap plug in it...
	 */

	if (uvm_map_submap(map, *min, *max, submap) != 0)
		panic("uvm_km_suballoc: submap allocation failed");

	return(submap);
}

/*
 * uvm_km_pgremove: remove pages from a kernel uvm_object.
 *
 * => when you unmap a part of anonymous kernel memory you want to toss
 *    the pages right away.    (this gets called from uvm_unmap_...).
 */
void
uvm_km_pgremove(struct uvm_object *uobj, vaddr_t start, vaddr_t end)
{
	struct vm_page *pp;
	voff_t curoff;
	int slot;

	KASSERT(uobj->pgops == &aobj_pager);

	for (curoff = start ; curoff < end ; curoff += PAGE_SIZE) {
		pp = uvm_pagelookup(uobj, curoff);
		if (pp && pp->pg_flags & PG_BUSY) {
			atomic_setbits_int(&pp->pg_flags, PG_WANTED);
			UVM_UNLOCK_AND_WAIT(pp, &uobj->vmobjlock, 0,
			    "km_pgrm", 0);
			simple_lock(&uobj->vmobjlock);
			curoff -= PAGE_SIZE; /* loop back to us */
			continue;
		}

		/* free the swap slot, then the page */
		slot = uao_dropswap(uobj, curoff >> PAGE_SHIFT);

		if (pp != NULL) {
			uvm_lock_pageq();
			uvm_pagefree(pp);
			uvm_unlock_pageq();
		} else if (slot != 0) {
			simple_lock(&uvm.swap_data_lock);
			uvmexp.swpgonly--;
			simple_unlock(&uvm.swap_data_lock);
		}
	}
}


/*
 * uvm_km_pgremove_intrsafe: like uvm_km_pgremove(), but for "intrsafe"
 *    objects
 *
 * => when you unmap a part of anonymous kernel memory you want to toss
 *    the pages right away.    (this gets called from uvm_unmap_...).
 * => none of the pages will ever be busy, and none of them will ever
 *    be on the active or inactive queues (because these objects are
 *    never allowed to "page").
 */

void
uvm_km_pgremove_intrsafe(vaddr_t start, vaddr_t end)
{
	struct vm_page *pg;
	vaddr_t va;
	paddr_t pa;

	for (va = start; va < end; va += PAGE_SIZE) {
		if (!pmap_extract(pmap_kernel(), va, &pa))
			continue;
		pg = PHYS_TO_VM_PAGE(pa);
		if (pg == NULL)
			panic("uvm_km_pgremove_intrsafe: no page");
		uvm_pagefree(pg);
	}
}

#if defined(__HAVE_PMAP_DIRECT)
/*
 * uvm_km_page allocator, __HAVE_PMAP_DIRECT arch
 * On architectures with machine memory direct mapped into a portion *
 of KVM, we have very little work to do.  Just get a physical page, *
 and find and return its VA.  */
void uvm_km_page_init(void) {
	/* nothing */
}

#else
/*
 * uvm_km_page allocator, non __HAVE_PMAP_DIRECT archs
 * This is a special allocator that uses a reserve of free pages
 * to fulfill requests.  It is fast and interrupt safe, but can only
 * return page sized regions.  Its primary use is as a backend for pool.
 *
 * The memory returned is allocated from the larger kernel_map, sparing
 * pressure on the small interrupt-safe kmem_map.  It is wired, but
 * not zero filled.
 */

struct uvm_km_pages uvm_km_pages;

void uvm_km_createthread(void *);
void uvm_km_thread(void *);
struct uvm_km_free_page *uvm_km_doputpage(struct uvm_km_free_page *);

/*
 * Allocate the initial reserve, and create the thread which will
 * keep the reserve full.  For bootstrapping, we allocate more than
 * the lowat amount, because it may be a while before the thread is
 * running.
 */
void
uvm_km_page_init(void)
{
	int	lowat_min;
	int	i;
	int	len, bulk;
	vaddr_t	addr;

	mtx_init(&uvm_km_pages.mtx, IPL_VM);
	if (!uvm_km_pages.lowat) {
		/* based on physmem, calculate a good value here */
		uvm_km_pages.lowat = physmem / 256;
		lowat_min = physmem < atop(16 * 1024 * 1024) ? 32 : 128;
		if (uvm_km_pages.lowat < lowat_min)
			uvm_km_pages.lowat = lowat_min;
	}
	if (uvm_km_pages.lowat > UVM_KM_PAGES_LOWAT_MAX)
		uvm_km_pages.lowat = UVM_KM_PAGES_LOWAT_MAX;
	uvm_km_pages.hiwat = 4 * uvm_km_pages.lowat;
	if (uvm_km_pages.hiwat > UVM_KM_PAGES_HIWAT_MAX)
		uvm_km_pages.hiwat = UVM_KM_PAGES_HIWAT_MAX;

	/* Allocate all pages in as few allocations as possible. */
	len = 0;
	bulk = uvm_km_pages.hiwat;
	while (len < uvm_km_pages.hiwat && bulk > 0) {
		bulk = MIN(bulk, uvm_km_pages.hiwat - len);
		addr = vm_map_min(kernel_map);
		if (uvm_map(kernel_map, &addr, (vsize_t)bulk << PAGE_SHIFT,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_NONE,
		    UVM_ADV_RANDOM, UVM_KMF_TRYLOCK)) != 0) {
			bulk /= 2;
			continue;
		}

		for (i = len; i < len + bulk; i++, addr += PAGE_SIZE)
			uvm_km_pages.page[i] = addr;
		len += bulk;
	}

	uvm_km_pages.free = len;
	for (i = len; i < UVM_KM_PAGES_HIWAT_MAX; i++)
		uvm_km_pages.page[i] = 0;

	/* tone down if really high */
	if (uvm_km_pages.lowat > 512)
		uvm_km_pages.lowat = 512;

	kthread_create_deferred(uvm_km_createthread, NULL);
}

void
uvm_km_createthread(void *arg)
{
	kthread_create(uvm_km_thread, NULL, &uvm_km_pages.km_proc, "kmthread");
}

/*
 * Endless loop.  We grab pages in increments of 16 pages, then
 * quickly swap them into the list.  At some point we can consider
 * returning memory to the system if we have too many free pages,
 * but that's not implemented yet.
 */
void
uvm_km_thread(void *arg)
{
	vaddr_t pg[16];
	int i;
	int allocmore = 0;
	struct uvm_km_free_page *fp = NULL;

	for (;;) {
		mtx_enter(&uvm_km_pages.mtx);
		if (uvm_km_pages.free >= uvm_km_pages.lowat &&
		    uvm_km_pages.freelist == NULL) {
			msleep(&uvm_km_pages.km_proc, &uvm_km_pages.mtx,
			    PVM, "kmalloc", 0);
		}
		allocmore = uvm_km_pages.free < uvm_km_pages.lowat;
		fp = uvm_km_pages.freelist;
		uvm_km_pages.freelist = NULL;
		uvm_km_pages.freelistlen = 0;
		mtx_leave(&uvm_km_pages.mtx);

		if (allocmore) {
			bzero(pg, sizeof(pg));
			for (i = 0; i < nitems(pg); i++) {
				pg[i] = vm_map_min(kernel_map);
				if (uvm_map(kernel_map, &pg[i], PAGE_SIZE,
				    NULL, UVM_UNKNOWN_OFFSET, 0,
				    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
				    UVM_INH_NONE, UVM_ADV_RANDOM,
				    UVM_KMF_TRYLOCK)) != 0) {
					pg[i] = 0;
					break;
				}
			}

			mtx_enter(&uvm_km_pages.mtx);
			for (i = 0; i < nitems(pg); i++) {
				if (uvm_km_pages.free ==
				    nitems(uvm_km_pages.page))
					break;
				else if (pg[i] != 0)
					uvm_km_pages.page[uvm_km_pages.free++]
					    = pg[i];
			}
			wakeup(&uvm_km_pages.free);
			mtx_leave(&uvm_km_pages.mtx);

			/* Cleanup left-over pages (if any). */
			for (; i < nitems(pg); i++) {
				if (pg[i] != 0) {
					uvm_unmap(kernel_map,
					    pg[i], pg[i] + PAGE_SIZE);
				}
			}
		}
		while (fp) {
			fp = uvm_km_doputpage(fp);
		}
	}
}

struct uvm_km_free_page *
uvm_km_doputpage(struct uvm_km_free_page *fp)
{
	vaddr_t va = (vaddr_t)fp;
	struct vm_page *pg;
	int	freeva = 1;
	struct uvm_km_free_page *nextfp = fp->next;

	pg = uvm_atopg(va);

	pmap_kremove(va, PAGE_SIZE);
	pmap_update(kernel_map->pmap);

	mtx_enter(&uvm_km_pages.mtx);
	if (uvm_km_pages.free < uvm_km_pages.hiwat) {
		uvm_km_pages.page[uvm_km_pages.free++] = va;
		freeva = 0;
	}
	mtx_leave(&uvm_km_pages.mtx);

	if (freeva)
		uvm_unmap(kernel_map, va, va + PAGE_SIZE);

	uvm_pagefree(pg);
	return (nextfp);
}
#endif	/* !__HAVE_PMAP_DIRECT */

void *
km_alloc(size_t sz, const struct kmem_va_mode *kv,
    const struct kmem_pa_mode *kp, const struct kmem_dyn_mode *kd)
{
	struct vm_map *map;
	struct vm_page *pg;
	struct pglist pgl;
	int mapflags = 0;
	vm_prot_t prot;
	int pla_flags;
	int pla_maxseg;
#ifdef __HAVE_PMAP_DIRECT
	paddr_t pa;
#endif
	vaddr_t va, sva;

	KASSERT(sz == round_page(sz));

	TAILQ_INIT(&pgl);

	if (kp->kp_nomem || kp->kp_pageable)
		goto alloc_va;

	pla_flags = kd->kd_waitok ? UVM_PLA_WAITOK : UVM_PLA_NOWAIT;
	pla_flags |= UVM_PLA_TRYCONTIG;
	if (kp->kp_zero)
		pla_flags |= UVM_PLA_ZERO;

	pla_maxseg = kp->kp_maxseg;
	if (pla_maxseg == 0)
		pla_maxseg = sz / PAGE_SIZE;

	if (uvm_pglistalloc(sz, kp->kp_constraint->ucr_low,
	    kp->kp_constraint->ucr_high, kp->kp_align, kp->kp_boundary,
	    &pgl, pla_maxseg, pla_flags)) {	
		return (NULL);
	}

#ifdef __HAVE_PMAP_DIRECT
	if (kv->kv_align || kv->kv_executable)
		goto alloc_va;
#if 1
	/*
	 * For now, only do DIRECT mappings for single page
	 * allocations, until we figure out a good way to deal
	 * with contig allocations in km_free.
	 */
	if (!kv->kv_singlepage)
		goto alloc_va;
#endif
	/*
	 * Dubious optimization. If we got a contig segment, just map it
	 * through the direct map.
	 */
	TAILQ_FOREACH(pg, &pgl, pageq) {
		if (pg != TAILQ_FIRST(&pgl) &&
		    VM_PAGE_TO_PHYS(pg) != pa + PAGE_SIZE)
			break;
		pa = VM_PAGE_TO_PHYS(pg);
	}
	if (pg == NULL) {
		TAILQ_FOREACH(pg, &pgl, pageq) {
			vaddr_t v;
			v = pmap_map_direct(pg);
			if (pg == TAILQ_FIRST(&pgl))
				va = v;
		}
		return ((void *)va);
	}
#endif
alloc_va:
	if (kv->kv_executable) {
		prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	} else {
		prot = VM_PROT_READ | VM_PROT_WRITE;
	}

	if (kp->kp_pageable) {
		KASSERT(kp->kp_object);
		KASSERT(!kv->kv_singlepage);
	} else {
		KASSERT(kp->kp_object == NULL);
	}

	if (kv->kv_singlepage) {
		KASSERT(sz == PAGE_SIZE);
#ifdef __HAVE_PMAP_DIRECT
		panic("km_alloc: DIRECT single page");
#else
		mtx_enter(&uvm_km_pages.mtx);
		while (uvm_km_pages.free == 0) {
			if (kd->kd_waitok == 0) {
				mtx_leave(&uvm_km_pages.mtx);
				uvm_pglistfree(&pgl);
				return NULL;
			}
			msleep(&uvm_km_pages.free, &uvm_km_pages.mtx, PVM,
			    "getpage", 0);
		}
		va = uvm_km_pages.page[--uvm_km_pages.free];
		if (uvm_km_pages.free < uvm_km_pages.lowat &&
		    curproc != uvm_km_pages.km_proc) {
			if (kd->kd_slowdown)
				*kd->kd_slowdown = 1;
			wakeup(&uvm_km_pages.km_proc);
		}
		mtx_leave(&uvm_km_pages.mtx);
#endif
	} else {
		struct uvm_object *uobj = NULL;

		if (kd->kd_trylock)
			mapflags |= UVM_FLAG_TRYLOCK;

		if (kp->kp_object)
			uobj = *kp->kp_object;
try_map:
		map = *kv->kv_map;
		va = vm_map_min(map);
		if (uvm_map(map, &va, sz, uobj, kd->kd_prefer,
		    kv->kv_align, UVM_MAPFLAG(prot, prot, UVM_INH_NONE,
		    UVM_ADV_RANDOM, mapflags))) {
			if (kv->kv_wait && kd->kd_waitok) {
				tsleep(map, PVM, "km_allocva", 0);
				goto try_map;
			}
			uvm_pglistfree(&pgl);
			return (NULL);
		}
	}
	sva = va;
	TAILQ_FOREACH(pg, &pgl, pageq) {
		if (kp->kp_pageable)
			pmap_enter(pmap_kernel(), va, VM_PAGE_TO_PHYS(pg),
			    prot, prot | PMAP_WIRED);
		else
			pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg), prot);
		va += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return ((void *)sva);
}

void
km_free(void *v, size_t sz, const struct kmem_va_mode *kv,
    const struct kmem_pa_mode *kp)
{
	vaddr_t sva, eva, va;
	struct vm_page *pg;
	struct pglist pgl;

	sva = va = (vaddr_t)v;
	eva = va + sz;

	if (kp->kp_nomem) {
		goto free_va;
	}

	if (kv->kv_singlepage) {
#ifdef __HAVE_PMAP_DIRECT
		pg = pmap_unmap_direct(va);
		uvm_pagefree(pg);
#else
		struct uvm_km_free_page *fp = v;
		mtx_enter(&uvm_km_pages.mtx);
		fp->next = uvm_km_pages.freelist;
		uvm_km_pages.freelist = fp;
		if (uvm_km_pages.freelistlen++ > 16)
			wakeup(&uvm_km_pages.km_proc);
		mtx_leave(&uvm_km_pages.mtx);
#endif
		return;
	}

	if (kp->kp_pageable) {
		pmap_remove(pmap_kernel(), sva, eva);
		pmap_update(pmap_kernel());
	} else {
		TAILQ_INIT(&pgl);
		for (va = sva; va < eva; va += PAGE_SIZE) {
			paddr_t pa;

			if (!pmap_extract(pmap_kernel(), va, &pa))
				continue;

			pg = PHYS_TO_VM_PAGE(pa);
			if (pg == NULL) {
				panic("km_free: unmanaged page 0x%lx\n", pa);
			}
			TAILQ_INSERT_TAIL(&pgl, pg, pageq);
		}
		pmap_kremove(sva, sz);
		pmap_update(pmap_kernel());
		uvm_pglistfree(&pgl);
	}
free_va:
	uvm_unmap(*kv->kv_map, sva, eva);
	if (kv->kv_wait)
		wakeup(*kv->kv_map);
}

const struct kmem_va_mode kv_any = {
	.kv_map = &kernel_map,
};

const struct kmem_va_mode kv_intrsafe = {
	.kv_map = &kmem_map,
};

const struct kmem_va_mode kv_page = {
	.kv_singlepage = 1
};

const struct kmem_pa_mode kp_dirty = {
	.kp_constraint = &no_constraint
};

const struct kmem_pa_mode kp_dma = {
	.kp_constraint = &dma_constraint
};

const struct kmem_pa_mode kp_dma_contig = {
	.kp_constraint = &dma_constraint,
	.kp_maxseg = 1
};

const struct kmem_pa_mode kp_dma_zero = {
	.kp_constraint = &dma_constraint,
	.kp_zero = 1
};

const struct kmem_pa_mode kp_zero = {
	.kp_constraint = &no_constraint,
	.kp_zero = 1
};

const struct kmem_pa_mode kp_pageable = {
	.kp_object = &uvm.kernel_object,
	.kp_pageable = 1
/* XXX - kp_nomem, maybe, but we'll need to fix km_free. */
};

const struct kmem_pa_mode kp_none = {
	.kp_nomem = 1
};

const struct kmem_dyn_mode kd_waitok = {
	.kd_waitok = 1,
	.kd_prefer = UVM_UNKNOWN_OFFSET
};

const struct kmem_dyn_mode kd_nowait = {
	.kd_prefer = UVM_UNKNOWN_OFFSET
};

const struct kmem_dyn_mode kd_trylock = {
	.kd_trylock = 1,
	.kd_prefer = UVM_UNKNOWN_OFFSET
};
