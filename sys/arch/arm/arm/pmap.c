/* $OpenBSD$ */

/*
 * Copyright (c) 2008-2009,2014 Dale Rahn <drahn@dalerahn.com>
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/pool.h>

#include <uvm/uvm.h>

#include "arm/vmparam.h"
#include "arm/pmap.h"
#include "machine/pcb.h"

#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>

struct pmap kernel_pmap_;

LIST_HEAD(pted_pv_head, pte_desc);

struct pte_desc {
        LIST_ENTRY(pte_desc) pted_pv_list;
	uint32_t pte;
	pmap_t pted_pmap;
	vaddr_t pted_va;
};

/* VP routines */
void pmap_vp_enter(pmap_t pm, vaddr_t va, struct pte_desc *pted);
struct pte_desc *pmap_vp_remove(pmap_t pm, vaddr_t va);
void pmap_vp_destroy(pmap_t pm);
struct pte_desc *pmap_vp_lookup(pmap_t pm, vaddr_t va);

/* PV routines */
void pmap_enter_pv(struct pte_desc *pted, struct vm_page *);
void pmap_remove_pv(struct pte_desc *pted);

void _pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, int flags,
    int cache);

void pmap_remove_pg(pmap_t pm, vaddr_t va);
void pmap_kremove_pg(vaddr_t va);


/* XXX */
void
pmap_fill_pte(pmap_t pm, vaddr_t va, paddr_t pa, struct pte_desc *pted,
    vm_prot_t prot, int flags, int cache);
void pte_insert(struct pte_desc *pted);
void pmap_table_remove(struct pte_desc *pted);
void pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable);
void pmap_table_insert(struct pte_desc *pted);
void pmap_pinit(pmap_t pm);
void pmap_release(pmap_t pm);
vaddr_t pmap_steal_memory(vsize_t size, vaddr_t *start, vaddr_t *end);
paddr_t arm_kvm_stolen;
void * pmap_steal_avail(size_t size, int align);
void pmap_remove_avail(paddr_t base, paddr_t end);


/* pte invalidation */
void pte_invalidate(void *ptp, struct pte_desc *pted);

/* XXX - panic on pool get failures? */
struct pool pmap_pmap_pool;
struct pool pmap_vp_pool;
struct pool pmap_pted_pool;

/* list of L1 tables */

int pmap_initialized = 0;

/* virtual to physical helpers */
static inline int
VP_IDX1(vaddr_t va)
{
	return (va >> VP_IDX1_POS) & VP_IDX1_MASK;
}

static inline int  
VP_IDX2(vaddr_t va)
{
	return (va >> VP_IDX2_POS) & VP_IDX2_MASK;
}

static inline int
VP_IDX3(vaddr_t va)   
{
	return (va >> VP_IDX3_POS) & VP_IDX3_MASK;
}


#if VP_IDX2_SIZE != VP_IDX3_SIZE
#error pmap allocation code expects IDX2 and IDX3 size to be same
#endif
struct pmapvp {
        void *vp[VP_IDX1_SIZE];
};

/*
 * This is used for pmap_kernel() mappings, they are not to be removed
 * from the vp table because they were statically initialized at the
 * initial pmap initialization. This is so that memory allocation
 * is not necessary in the pmap_kernel() mappings.
 * Otherwise bad race conditions can appear.
 */
struct pte_desc *
pmap_vp_lookup(pmap_t pm, vaddr_t va)
{
	struct pmapvp *vp1;
	struct pmapvp *vp2;  
	struct pte_desc *pted;

	vp1 = pm->pm_vp[VP_IDX1(va)];
	if (vp1 == NULL) {
		return NULL;
	}
	    
	vp2 = vp1->vp[VP_IDX2(va)];
	if (vp2 == NULL) {
		return NULL;
	}
	    
	pted = vp2->vp[VP_IDX3(va)];

	return pted;
}

/*
 * Remove, and return, pted at specified address, NULL if not present
 */
struct pte_desc *
pmap_vp_remove(pmap_t pm, vaddr_t va)
{
	struct pmapvp *vp1;
	struct pmapvp *vp2;
	struct pte_desc *pted;

	vp1 = pm->pm_vp[VP_IDX1(va)];
	if (vp1 == NULL) {
		return NULL;
	}
	    
	vp2 = vp1->vp[VP_IDX2(va)];
	if (vp2 == NULL) {
		return NULL;
	}
	    
	pted = vp2->vp[VP_IDX3(va)];
	vp2->vp[VP_IDX3(va)] = NULL;

	return pted;
}

/*
 * Create a V -> P mapping for the given pmap and virtual address
 * with reference to the pte descriptor that is used to map the page.
 * This code should track allocations of vp table allocations
 * so they can be freed efficiently.
 */
void
pmap_vp_enter(pmap_t pm, vaddr_t va, struct pte_desc *pted)
{
	struct pmapvp *vp1;
	struct pmapvp *vp2;
	int s;

	vp1 = pm->pm_vp[VP_IDX1(va)];
	if (vp1 == NULL) {
		s = splvm();
		vp1 = pool_get(&pmap_vp_pool, PR_NOWAIT | PR_ZERO);
		splx(s);
		pm->pm_vp[VP_IDX1(va)] = vp1;
	}

	vp2 = vp1->vp[VP_IDX2(va)];
	if (vp2 == NULL) {
		s = splvm();
		vp2 = pool_get(&pmap_vp_pool, PR_NOWAIT | PR_ZERO);
		splx(s);
		vp1->vp[VP_IDX2(va)] = vp2;
	}

	vp2->vp[VP_IDX3(va)] = pted;
}


u_int32_t PTED_MANAGED(struct pte_desc *pted);
u_int32_t PTED_WIRED(struct pte_desc *pted);
u_int32_t PTED_VALID(struct pte_desc *pted);

u_int32_t
PTED_MANAGED(struct pte_desc *pted)
{
	return (pted->pted_va & PTED_VA_MANAGED_M);
}

u_int32_t
PTED_WIRED(struct pte_desc *pted)
{
	return (pted->pted_va & PTED_VA_WIRED_M);
}

u_int32_t
PTED_VALID(struct pte_desc *pted)
{
	return (pted->pte != 0);
}

/*
 * PV entries -
 * manipulate the physical to virtual translations for the entire system.
 *
 * QUESTION: should all mapped memory be stored in PV tables? Or
 * is it alright to only store "ram" memory. Currently device mappings
 * are not stored.
 * It makes sense to pre-allocate mappings for all of "ram" memory, since
 * it is likely that it will be mapped at some point, but would it also
 * make sense to use a tree/table like is use for pmap to store device
 * mappings?
 * Further notes: It seems that the PV table is only used for pmap_protect
 * and other paging related operations. Given this, it is not necessary
 * to store any pmap_kernel() entries in PV tables and does not make
 * sense to store device mappings in PV either.
 *
 * Note: unlike other powerpc pmap designs, the array is only an array
 * of pointers. Since the same structure is used for holding information
 * in the VP table, the PV table, and for kernel mappings, the wired entries.
 * Allocate one data structure to hold all of the info, instead of replicating
 * it multiple times.
 *
 * One issue of making this a single data structure is that two pointers are
 * wasted for every page which does not map ram (device mappings), this
 * should be a low percentage of mapped pages in the system, so should not
 * have too noticable unnecessary ram consumption.
 */

void
pmap_enter_pv(struct pte_desc *pted, struct vm_page *pg)
{
	if (__predict_false(!pmap_initialized)) {
		return;
	}

	LIST_INSERT_HEAD(&(pg->mdpage.pv_list), pted, pted_pv_list);
	pted->pted_va |= PTED_VA_MANAGED_M;
}

void
pmap_remove_pv(struct pte_desc *pted)
{
	LIST_REMOVE(pted, pted_pv_list);
}

int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct pte_desc *pted;
	struct vm_page *pg;
	int s;
	int need_sync = 0;
	int cache;

	/* MP - Acquire lock for this pmap */

	s = splvm();
	pted = pmap_vp_lookup(pm, va);
	if (pted && PTED_VALID(pted)) {
		pmap_remove_pg(pm, va);
		/* we lost our pted if it was user */
		if (pm != pmap_kernel())
			pted = pmap_vp_lookup(pm, va);
	}

	pm->pm_stats.resident_count++;

	/* Do not have pted for this, get one and put it in VP */
	if (pted == NULL) {
		pted = pool_get(&pmap_pted_pool, PR_NOWAIT | PR_ZERO);
		pmap_vp_enter(pm, va, pted);
	}

	/* Calculate PTE */
	pg = PHYS_TO_VM_PAGE(pa);
	if (pg != NULL) {
		/* max cacheable */
		cache = PMAP_CACHE_WB; /* managed memory is cacheable */
	} else {
		cache = PMAP_CACHE_CI;
	}

	pmap_fill_pte(pm, va, pa, pted, prot, flags, cache);

	if (pg != NULL) {
		pmap_enter_pv(pted, pg); /* only managed mem */
	}

	/*
	 * Insert into table.
	 */
	pte_insert(pted);

	if (prot & VM_PROT_EXECUTE) {
		if (pg != NULL) {
			need_sync = ((pg->pg_flags & PG_PMAP_EXE) == 0);
			atomic_setbits_int(&pg->pg_flags, PG_PMAP_EXE);
		} else
			need_sync = 1;
	} else {
		/*
		 * Should we be paranoid about writeable non-exec
		 * mappings ? if so, clear the exec tag
		 */
		if ((prot & VM_PROT_WRITE) && (pg != NULL))
			atomic_clearbits_int(&pg->pg_flags, PG_PMAP_EXE);
	}

	splx(s);

#if 0
	/* only instruction sync executable pages */
	if (need_sync)
		pmap_syncicache_user_virt(pm, va);
#endif

	/* MP - free pmap lock */
	return 0;
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pmap_t pm, vaddr_t va, vaddr_t endva)
{
	int i_vp1, s_vp1, e_vp1;
	int i_vp2, s_vp2, e_vp2;
	int i_vp3, s_vp3, e_vp3;
	struct pmapvp *vp2;
	struct pmapvp *vp3;
	
	/* I suspect that if this loop were unrolled better
	 * it would have better performance, testing i_vp1 and i_vp2
	 * in the middle loop seems excessive
	 */
		
	s_vp1 = VP_IDX1(va);
	e_vp1 = VP_IDX1(endva);
	for (i_vp1 = s_vp1; i_vp1 <= e_vp1; i_vp1++) {
		vp2 = pm->pm_vp[i_vp1];
		if (vp2 == NULL)
			continue;
	    
		if (i_vp1 == s_vp1)
			s_vp2 = VP_IDX2(va);
		else
			s_vp2 = 0;
	    
		if (i_vp1 == e_vp1)
			e_vp2 = VP_IDX2(endva);
		else
			e_vp2 = VP_IDX2_SIZE-1;
	 
		for (i_vp2 = s_vp2; i_vp2 <= e_vp2; i_vp2++) {
			vp3 = vp2->vp[i_vp2];
			if (vp3 == NULL)
				continue;
  
			if ((i_vp1 == s_vp1) && (i_vp2 == s_vp2))
				s_vp3 = VP_IDX3(va);
			else
				s_vp3 = 0;
    
			if ((i_vp1 == e_vp1) && (i_vp2 == e_vp2))
				e_vp3 = VP_IDX3(endva);
			else
				e_vp3 = VP_IDX3_SIZE;
 
			for (i_vp3 = s_vp3; i_vp3 < e_vp3; i_vp3++) {
				if (vp3->vp[i_vp3] != NULL) {
					pmap_remove_pg(pm,
					    (i_vp1 << VP_IDX1_POS) |
					    (i_vp2 << VP_IDX2_POS) |
					    (i_vp3 << VP_IDX3_POS));
				}
			}
		}
	}
}

/*
 * remove a single mapping, notice that this code is O(1)
 */
void
pmap_remove_pg(pmap_t pm, vaddr_t va)
{        
	struct pte_desc *pted;
	int s;
 
	s = splvm();
	if (pm == pmap_kernel()) {
		pted = pmap_vp_lookup(pm, va);
		if (pted == NULL || !PTED_VALID(pted)) {
			splx(s);
			return;
		}
	} else {
		pted = pmap_vp_remove(pm, va);
		if (pted == NULL || !PTED_VALID(pted)) {
			splx(s);
			return;
		}
	}
	pm->pm_stats.resident_count--;

	pmap_table_remove(pted);

	if (pted->pted_va & PTED_VA_EXEC_M) {
		pted->pted_va &= ~PTED_VA_EXEC_M;
	}

	pted->pte = 0;

	if (PTED_MANAGED(pted))
		pmap_remove_pv(pted);

	if (pm != pmap_kernel())
		pool_put(&pmap_pted_pool, pted);

	splx(s);
}

/*      
 * Enter a kernel mapping for the given page.
 * kernel mappings have a larger set of prerequisites than normal mappings.
 *
 * 1. no memory should be allocated to create a kernel mapping.
 * 2. a vp mapping should already exist, even if invalid. (see 1)
 * 3. all vp tree mappings should already exist (see 1)
 *
 */
void
_pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, int flags, int cache)
{
	struct pte_desc *pted;
	int s;
	pmap_t pm;
	
	pm = pmap_kernel();
    
	/* MP - lock pmap. */
	s = splvm();
	
	pted = pmap_vp_lookup(pm, va);
	if (pted && PTED_VALID(pted))
		pmap_kremove_pg(va); /* pted is reused */ 
					    
	pm->pm_stats.resident_count++;
					    
	/* Do not have pted for this, get one and put it in VP */
	if (pted == NULL) {
		panic("pted not preallocated in pmap_kernel() va %lx pa %lx\n",
		    va, pa);
	}
  
	if (cache == PMAP_CACHE_DEFAULT) {
		if (PHYS_TO_VM_PAGE(pa) != NULL) {
			/* MAXIMUM cacheability */
			cache = PMAP_CACHE_WB; /* managed memory is cacheable */
		} else
			cache = PMAP_CACHE_CI;
	}
	
	/* Calculate PTE */
	pmap_fill_pte(pm, va, pa, pted, prot, flags, cache);
	   
	/*
	 * Insert into table
	 * We were told to map the page, probably called from vm_fault,
	 * so map the page!
	 */
	pte_insert(pted);

	pted->pted_va |= PTED_VA_WIRED_M;


	splx(s);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
        _pmap_kenter_pa(va, pa, prot, 0, PMAP_CACHE_DEFAULT);
}

void
pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable)
{
        _pmap_kenter_pa(va, pa, prot, 0, cacheable);
}

/*
 * remove kernel (pmap_kernel()) mapping, one page
 */
void
pmap_kremove_pg(vaddr_t va)
{
	struct pte_desc *pted;
	pmap_t pm;
	int s;

	pm = pmap_kernel();
	pted = pmap_vp_lookup(pm, va);
	if (pted == NULL)
		return;

	if (!PTED_VALID(pted))
		return; /* not mapped */

	s = splvm();

	pm->pm_stats.resident_count--;

	/*
	 * Table needs to be locked here as well as pmap, and pv list.
	 * so that we know the mapping information is either valid,
	 * or that the mapping is not present in the hash table.
	 */
	pmap_table_remove(pted);

	if (pted->pted_va & PTED_VA_EXEC_M)
		pted->pted_va &= ~PTED_VA_EXEC_M;

	if (PTED_MANAGED(pted))
		pmap_remove_pv(pted);

	/* invalidate pted; */
	pted->pte = 0;

	splx(s);

}

/*
 * remove kernel (pmap_kernel()) mappings
 */
void
pmap_kremove(vaddr_t va, vsize_t len)
{
	for (len >>= PAGE_SHIFT; len >0; len--, va += PAGE_SIZE)
		pmap_kremove_pg(va);
}

void
pte_invalidate(void *ptp, struct pte_desc *pted)
{
	panic("implement pte_invalidate");
}


void
pmap_table_insert(struct pte_desc *pted)
{
	panic("implement pmap_table_insert");
}

void
pmap_table_remove(struct pte_desc *pted)
{
	panic("implement pmap_table_insert");
}

void
pmap_fill_pte(pmap_t pm, vaddr_t va, paddr_t pa, struct pte_desc *pted,
    vm_prot_t prot, int flags, int cache)
{
}


/*
 * Garbage collects the physical map system for pages which are
 * no longer used. Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but others may be collected
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap_t pm)
{
	/* This could return unused v->p table layers which
	 * are empty.
	 * could malicious programs allocate memory and eat
	 * these wired pages? These are allocated via pool.
	 * Are there pool functions which could be called
	 * to lower the pool usage here?
	 */
}

/*
 * Fill the given physical page with zeros.
 * XXX
 */
void
pmap_zero_page(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	/* simple_lock(&pmap_zero_page_lock); */  
	pmap_kenter_pa(zero_page, pa, VM_PROT_READ|VM_PROT_WRITE);

	bzero((void *)zero_page, PAGE_SIZE);

	pmap_kremove_pg(zero_page);
  
}

/*
 * copy the given physical page with zeros.
 */
void
pmap_copy_page(struct vm_page *srcpg, struct vm_page *dstpg)
{
	paddr_t srcpa = VM_PAGE_TO_PHYS(srcpg);
	paddr_t dstpa = VM_PAGE_TO_PHYS(dstpg);
	/* simple_lock(&pmap_copy_page_lock); */
			   
	pmap_kenter_pa(copy_src_page, srcpa, VM_PROT_READ);
	pmap_kenter_pa(copy_dst_page, dstpa, VM_PROT_READ|VM_PROT_WRITE);
  
	bcopy((void *)copy_src_page, (void *)copy_dst_page, PAGE_SIZE);

	pmap_kremove_pg(copy_src_page);
	pmap_kremove_pg(copy_dst_page);
	/* simple_unlock(&pmap_copy_page_lock); */
}


void
pmap_pinit(pmap_t pm)
{
#if 0
        int i, k, try, tblidx, tbloff;
        int s, seg;
#endif

        bzero(pm, sizeof (struct pmap));
                  
        pmap_reference(pm);
}

/*
 * Create and return a physical map.
 */
pmap_t
pmap_create()
{
	pmap_t pmap;
	int s;

	s = splvm();
	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	splx(s);
	pmap_pinit(pmap);
	return (pmap);
}

/*
 * Add a reference to a given pmap.
 */
void
pmap_reference(pmap_t pm)
{
	/* simple_lock(&pmap->pm_obj.vmobjlock); */
	pm->pm_refs++;
	/* simple_unlock(&pmap->pm_obj.vmobjlock); */
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pm)
{
	int refs;
	int s;

	/* simple_lock(&pmap->pm_obj.vmobjlock); */
	refs = --pm->pm_refs;
	/* simple_unlock(&pmap->pm_obj.vmobjlock); */
	if (refs > 0)
		return;

	/*
	 * reference count is zero, free pmap resources and free pmap.
	 */
	pmap_release(pm);
	s = splvm();
	pool_put(&pmap_pmap_pool, pm);
	splx(s);
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 */
void
pmap_release(pmap_t pm)
{
	pmap_vp_destroy(pm);
}

void
pmap_vp_destroy(pmap_t pm)
{
	int i, j;
	int s;
	struct pmapvp *vp1;
	struct pmapvp *vp2;

	for (i = 0; i < VP_IDX1_SIZE; i++) {
		vp1 = pm->pm_vp[i];
		if (vp1 == NULL)
			continue;

		for (j = 0; j < VP_IDX2_SIZE; j++) {
			vp2 = vp1->vp[j];
			if (vp2 == NULL)
				continue;

			s = splvm();
			pool_put(&pmap_vp_pool, vp2);
			splx(s);
		}
		pm->pm_vp[i] = NULL;
		s = splvm();
		pool_put(&pmap_vp_pool, vp1);
		splx(s);
	}
}

/*
 * Similar to pmap_steal_avail, but operating on vm_physmem since
 * uvm_page_physload() has been called.
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *start, vaddr_t *end)
{
	int segno;
	u_int npg;
	vaddr_t va;
	paddr_t pa;
	struct vm_physseg *seg;

	size = round_page(size);
	npg = atop(size);

	for (segno = 0, seg = vm_physmem; segno < vm_nphysseg; segno++, seg++) {
		if (seg->avail_end - seg->avail_start < npg)
			continue;
		/*
		 * We can only steal at an ``unused'' segment boundary,
		 * i.e. either at the start or at the end.
		 */
		if (seg->avail_start == seg->start ||
		    seg->avail_end == seg->end)
			break;
	}
	if (segno == vm_nphysseg)
		va = 0;
	else {
		if (seg->avail_start == seg->start) {
			pa = ptoa(seg->avail_start);
			seg->avail_start += npg;
			seg->start += npg;
		} else {
			pa = ptoa(seg->avail_end) - size;
			seg->avail_end -= npg;
			seg->end -= npg;
		}
		/*
		 * If all the segment has been consumed now, remove it.
		 * Note that the crash dump code still knows about it
		 * and will dump it correctly.
		 */
		if (seg->start == seg->end) {
			if (vm_nphysseg-- == 1)
				panic("pmap_steal_memory: out of memory");
			while (segno < vm_nphysseg) {
				seg[0] = seg[1]; /* struct copy */
				seg++;
				segno++;
			}
		}

		va = (vaddr_t)pa;       /* 1:1 mapping */
		bzero((void *)va, size);
	}

	if (start != NULL)
		*start = VM_MIN_KERNEL_ADDRESS;
	if (end != NULL)
		*end = VM_MAX_KERNEL_ADDRESS;

	return (va);
}


void pmap_setup_avail( uint32_t ram_start, uint32_t ram_end);
/*
 * Initialize pmap setup.
 * ALL of the code which deals with avail needs rewritten as an actual
 * memory allocation.
 */
void
pmap_bootstrap(u_int kernelstart, u_int kernelend, uint32_t ram_start,
    uint32_t ram_end)
{
	vaddr_t kvo;

	kvo = KERNEL_BASE_VIRT - KERNEL_BASE_PHYS;

	pmap_setup_avail(ram_start, ram_end);
	pmap_remove_avail(kernelstart-kvo, kernelend-kvo);


	/* XXX */

	zero_page = VM_MIN_KERNEL_ADDRESS + arm_kvm_stolen;
	arm_kvm_stolen += PAGE_SIZE;
	copy_src_page = VM_MIN_KERNEL_ADDRESS + arm_kvm_stolen;
	arm_kvm_stolen += PAGE_SIZE;
	copy_dst_page = VM_MIN_KERNEL_ADDRESS + arm_kvm_stolen;
	arm_kvm_stolen += PAGE_SIZE;
#if 0
	arm_kvm_stolen += reserve_dumppages( (caddr_t)(VM_MIN_KERNEL_ADDRESS +
	    arm_kvm_stolen));
#endif


	/* XXX */
}

/*
 * activate a pmap entry
 */
void
pmap_activate(struct proc *p)
{
	/* steal an L1 table if necessary */
}

/*
 * deactivate a pmap entry
 * NOOP on powerpc
 */
void
pmap_deactivate(struct proc *p)
{
	/* LRU */
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
boolean_t
pmap_extract(pmap_t pm, vaddr_t va, paddr_t *pa)
{
	struct pte_desc *pted;

	pted = pmap_vp_lookup(pm, va);

	if (pted == NULL)
		return FALSE;
	
	if (pted->pte == 0)
		return FALSE;

	*pa = (pted->pte& PTE_RPGN) | (va & ~PTE_RPGN);

	return TRUE;
}


/*
int
copyin(const void *udaddr, void *kaddr, size_t len)
{
	return 0;
}

int
copyout(const void *kaddr, void *udaddr, size_t len)
{
	return 0;
}

int
copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *done)
{
	return 0;
}

int
copyoutstr(const void *kaddr, void *udaddr, size_t len, size_t *done)
{
	return 0;
}
*/

void
pmap_page_ro(pmap_t pm, vaddr_t va, vm_prot_t prot)
{
	return;
}


/*
 * Lower the protection on the specified physical page.
 *
 * There are only two cases, either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	int s;
	struct pte_desc *pted;

	/* need to lock for this pv */
	s = splvm();

	if (prot == VM_PROT_NONE) {
		while (!LIST_EMPTY(&(pg->mdpage.pv_list))) {
			pted = LIST_FIRST(&(pg->mdpage.pv_list));
			pmap_remove_pg(pted->pted_pmap, pted->pted_va);
		}
		/* page is being reclaimed, sync icache next use */
		atomic_clearbits_int(&pg->pg_flags, PG_PMAP_EXE);
		splx(s);
		return;
	}

	LIST_FOREACH(pted, &(pg->mdpage.pv_list), pted_pv_list) {
		pmap_page_ro(pted->pted_pmap, pted->pted_va, prot);
	}
	splx(s);
}


void
pmap_protect(pmap_t pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	int s;
	if (prot & (VM_PROT_READ | VM_PROT_EXECUTE)) {
		s = splvm();
		while (sva < eva) {
			pmap_page_ro(pm, sva, 0);
			sva += PAGE_SIZE;
		}
		splx(s);
		return;
	}
	pmap_remove(pm, sva, eva);
}

#if 0
/*
 * Restrict given range to physical memory
 */
void
pmap_real_memory(paddr_t *start, vsize_t *size)
{
	struct mem_region *mp;

	for (mp = pmap_mem; mp->size; mp++) {
		if (((*start + *size) > mp->start)
			&& (*start < (mp->start + mp->size)))
		{
			if (*start < mp->start) {
				*size -= mp->start - *start;
				*start = mp->start;
			}
			if ((*start + *size) > (mp->start + mp->size))
				*size = mp->start + mp->size - *start;
			return;
		}
	}
	*size = 0;
}
#endif

void
pmap_init()
{
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmap", NULL);
	pool_setlowat(&pmap_pmap_pool, 2);
	pool_init(&pmap_vp_pool, sizeof(struct pmapvp), 0, 0, 0, "vp", NULL);
	pool_setlowat(&pmap_vp_pool, 10);
	pool_init(&pmap_pted_pool, sizeof(struct pte_desc), 0, 0, 0, "pted",
	    NULL);
	pool_setlowat(&pmap_pted_pool, 20);

	pmap_initialized = 1;
}

void
pmap_proc_iflush(struct proc *p, vaddr_t addr, vsize_t len)
{
	paddr_t pa;
	vsize_t clen;

	while (len > 0) {
		/* add one to always round up to the next page */
		clen = round_page(addr + 1) - addr;
		if (clen > len)
			clen = len;

		if (pmap_extract(p->p_vmspace->vm_map.pmap, addr, &pa)) {
			//syncicache((void *)pa, clen);
		}

		len -= clen;
		addr += clen;
	}
}

#if 0
int
pte_spill_v(pmap_t pm, u_int32_t va, u_int32_t dsisr, int exec_fault)
{
        struct pte_desc *pted;

        pted = pmap_vp_lookup(pm, va);
        if (pted == NULL) {
                return 0;
        }

        /*
         * if the current mapping is RO and the access was a write
         * we return 0
         */
        if (!PTED_VALID(pted)) {
                return 0;
        }

	/* check write fault and we have a readonly mapping */
	if ((dsisr & (1 << (31-6))) &&
	    (pted->pted_pte32.pte_lo & 0x1))
		return 0;

        if ((exec_fault != 0)
            && ((pted->pted_va & PTED_VA_EXEC_M) == 0)) {
                /* attempted to execute non-executable page */
                return 0;
        }

	pte_insert(pted);

        return 1;
}
	#endif

void
pte_insert(struct pte_desc *pted)
{
	/* put entry into table */
	/* need to deal with ref/change here */

}

int     pmap_fault_fixup(pmap_t pm0, vaddr_t v0, vm_prot_t p0, int i0)
{
return 0;
}
void pmap_postinit(void) {}
void    pmap_map_section(vaddr_t v0, vaddr_t v1, paddr_t p0, int i0, int i1) {}
void    pmap_map_entry(vaddr_t v0, vaddr_t v1, paddr_t p0, int i0, int i1) {}

vsize_t pmap_map_chunk(vaddr_t v0, vaddr_t v1, paddr_t p0, vsize_t s0, int prot, int cache)
{
	return 0;
}


void pmap_update()
{
}

void memhook(void);

void memhook()
{
}

void    pmap_link_l2pt(vaddr_t v0, vaddr_t v1, pv_addr_t * p1) {}

paddr_t zero_page;
paddr_t copy_src_page;
paddr_t copy_dst_page;


int pmap_is_referenced(struct vm_page *v0);
int pmap_is_referenced(struct vm_page *v0)
{
return 0;
}

int pmap_is_modified(struct vm_page *v0);
int pmap_is_modified(struct vm_page *v0)
{
return 0;
}

int pmap_clear_modify(struct vm_page *v0);
int pmap_clear_modify(struct vm_page *v0)
{
return 0;
}

int pmap_clear_reference(struct vm_page *v0);
int pmap_clear_reference(struct vm_page *v0)
{
return 0;
}

void pmap_copy(pmap_t src_pmap, pmap_t dst_pmap, vaddr_t src, vsize_t sz, vaddr_t dst)
{
}

void pmap_unwire(pmap_t pm, vaddr_t va)
{
}

void pmap_remove_holes(struct vm_map *v0)
{
}

void pmap_virtual_space(vaddr_t *va, vaddr_t *va1)
{
}

vaddr_t  pmap_curmaxkvaddr;

void    vector_page_setprot(int a)
{
}

void    pmap_set_pcb_pagedir(pmap_t pm, struct pcb *pcb)
{
}

struct mem_region {
	vaddr_t start;
	vsize_t size;
};

void pmap_avail_fixup(void);

struct mem_region pmap_avail_regions[10];
struct mem_region pmap_allocated_regions[10];
struct mem_region *pmap_avail = &pmap_avail_regions[0];
struct mem_region *pmap_allocated = &pmap_allocated_regions[0];
int pmap_cnt_avail, pmap_cnt_allocated;

void
pmap_setup_avail( uint32_t ram_start, uint32_t ram_end)
{
	pmap_avail[0].start = ram_start;
	pmap_avail[0].size = ram_end-ram_start;
	physmem = atop(pmap_avail[0].size);

	pmap_cnt_avail = 1;

	physmem = 0;

	pmap_avail_fixup();
}

void
pmap_avail_fixup(void)
{
	struct mem_region *mp;
	u_int32_t align;
	u_int32_t end;

	mp = pmap_avail;
	while(mp->size !=0) {
		align = round_page(mp->start);
		if (mp->start != align) {
			pmap_remove_avail(mp->start, align);
			mp = pmap_avail;
			continue;
		}
		end = mp->start+mp->size;
		align = trunc_page(end);
		if (end != align) {
			pmap_remove_avail(align, end);
			mp = pmap_avail;
			continue;
		}
		mp++;
	}
}

/* remove a given region from avail memory */
void
pmap_remove_avail(paddr_t base, paddr_t end)
{
	struct mem_region *mp;
	int i;
	int mpend;

	/* remove given region from available */
	for (mp = pmap_avail; mp->size; mp++) {
		/*
		 * Check if this region holds all of the region
		 */
		mpend = mp->start + mp->size;
		if (base > mpend) {
			continue;
		}
		if (base <= mp->start) {
			if (end <= mp->start)
				break; /* region not present -??? */

			if (end >= mpend) {
				/* covers whole region */
				/* shorten */
				for (i = mp - pmap_avail;
				    i < pmap_cnt_avail;
				    i++) {
					pmap_avail[i] = pmap_avail[i+1];
				}
				pmap_cnt_avail--;
				pmap_avail[pmap_cnt_avail].size = 0;
			} else {
				mp->start = end;
				mp->size = mpend - end;
			}
		} else {
			/* start after the beginning */
			if (end >= mpend) {
				/* just truncate */
				mp->size = base - mp->start;
			} else {
				/* split */
				for (i = pmap_cnt_avail;
				    i > (mp - pmap_avail);
				    i--) {
					pmap_avail[i] = pmap_avail[i - 1];
				}
				pmap_cnt_avail++;
				mp->size = base - mp->start;
				mp++;
				mp->start = end;
				mp->size = mpend - end;
			}
		}
	}
	for (mp = pmap_allocated; mp->size != 0; mp++) {
		if (base < mp->start) {
			if (end == mp->start) {
				mp->start = base;
				mp->size += end - base;
				break;
			}
			/* lengthen */
			for (i = pmap_cnt_allocated; i > (mp - pmap_allocated);
			    i--) {
				pmap_allocated[i] = pmap_allocated[i - 1];
			}
			pmap_cnt_allocated++;
			mp->start = base;
			mp->size = end - base;
			return;
		}
		if (base == (mp->start + mp->size)) {
			mp->size += end - base;
			return;
		}
	}
	if (mp->size == 0) {
		mp->start = base;
		mp->size  = end - base;
		pmap_cnt_allocated++;
	}
}

void *
pmap_steal_avail(size_t size, int align)
{
	struct mem_region *mp;
	int start;
	int remsize;

	for (mp = pmap_avail; mp->size; mp++) {
		if (mp->size > size) {
			start = (mp->start + (align -1)) & ~(align -1);
			remsize = mp->size - (start - mp->start); 
			if (remsize >= 0) {
				pmap_remove_avail(start, start+size);
				return (void *)start;
			}
		}
	}
	panic ("unable to allocate region with size %x align %x",
	    size, align);
}
