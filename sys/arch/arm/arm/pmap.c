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

/* FIXME FROM FREEBSD */
/* Write back D-cache to PoU */
static __inline void
dcache_wb_pou(vaddr_t va, vsize_t size)
{
	vaddr_t eva = va + size;

	__asm __volatile("dsb");
	va &= ~arm_dcache_min_line_mask;
	for ( ; va < eva; va += arm_dcache_min_line_size) {
#ifdef MULTIPROCESSOR
		__asm __volatile("mcr p15, 0, %0, c7, c11, 1" :: "r" (va));
#else
		__asm __volatile("mcr p15, 0, %0, c7, c10, 1" :: "r" (va));
#endif
	}
	__asm __volatile("dsb");
}

static __inline void
ttlb_flush(vaddr_t va)
{
	__asm __volatile("dsb");
	__asm __volatile("mcr p15, 0, %0, c8, c3, 3" :: "r" (va));
	__asm __volatile("dsb");
}

static __inline void
ttlb_flush_range(vaddr_t va, vsize_t size)
{
	vaddr_t eva = va + size;

	__asm __volatile("dsb");
	for ( ; va < eva; va += PAGE_SIZE)
		__asm __volatile("mcr p15, 0, %0, c8, c3, 3" :: "r" (va));
	__asm __volatile("dsb");
}

/*
 *  Clean L1 data cache range by physical address.
 *  The range must be within a single page.
 */
void pmap_kremove_pg(vaddr_t va);
static void
pmap_dcache_wb_pou(paddr_t pa, vsize_t size)
{
	int s;

	if (((pa & PAGE_MASK) + size) > PAGE_SIZE)
	    panic("%s: not on single page", __func__);

	s = splvm();
	pmap_kenter_pa(icache_page, pa, PROT_READ|PROT_WRITE);
	ttlb_flush(icache_page); // XXX tlb_flush_local
	dcache_wb_pou(icache_page + (pa & PAGE_MASK), size);
	pmap_kremove_pg(icache_page);
	splx(s);
}

/*
 *  Sync instruction cache range which is not mapped yet.
 */
void
cache_icache_sync_fresh(vaddr_t va, paddr_t pa, vsize_t size)
{
	uint32_t len, offset;

	/* Write back d-cache on given address range. */
	offset = pa & PAGE_MASK;
	for ( ; size != 0; size -= len, pa += len, offset = 0) {
		len = min(PAGE_SIZE - offset, size);
		pmap_dcache_wb_pou(pa, len);
	}
	/*
	 * I-cache is VIPT. Only way how to flush all virtual mappings
	 * on given physical address is to invalidate all i-cache.
	 */
	/* TODO: invalidate VA only? */
	//__asm __volatile("mcr p15, 0, r0, c7, c1,  0 /* Instruction cache invalidate all PoU, IS */"); MP
	__asm __volatile("mcr p15, 0, r0, c7, c5,  0 /* Instruction cache invalidate all PoU */");
	__asm __volatile("dsb");
	__asm __volatile("isb");
}

struct pmap kernel_pmap_;

LIST_HEAD(pted_pv_head, pte_desc);

struct pte_desc {
	LIST_ENTRY(pte_desc) pted_pv_list;
	uint32_t pted_pte;
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
void pmap_set_l2(struct pmap *pm, uint32_t pa, vaddr_t va, uint32_t l2_pa);


/* XXX */
void
pmap_fill_pte(pmap_t pm, vaddr_t va, paddr_t pa, struct pte_desc *pted,
    vm_prot_t prot, int flags, int cache);
void pte_insert(struct pte_desc *pted);
void pte_remove(struct pte_desc *pted);
void pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable);
void pmap_pinit(pmap_t pm);
void pmap_release(pmap_t pm);
//vaddr_t pmap_steal_memory(vsize_t size, vaddr_t *start, vaddr_t *end);
paddr_t arm_kvm_stolen;
void * pmap_steal_avail(size_t size, int align, void **kva);
void pmap_remove_avail(paddr_t base, paddr_t end);
void pmap_avail_fixup(void);
vaddr_t pmap_map_stolen(void);
void pmap_physload_avail(void);
extern caddr_t msgbufaddr;

/* TODO: ttb flags */
static uint32_t ttb_flags = 0;

/* pte invalidation */
void pte_invalidate(void *ptp, struct pte_desc *pted);

/* XXX - panic on pool get failures? */
struct pool pmap_pmap_pool;
struct pool pmap_vp_pool;
struct pool pmap_pted_pool;
struct pool pmap_l2_pool;

/* list of L1 tables */

int pmap_initialized = 0;

struct mem_region {
	vaddr_t start;
	vsize_t size;
};

struct mem_region pmap_avail_regions[10];
struct mem_region pmap_allocated_regions[10];
struct mem_region *pmap_avail = &pmap_avail_regions[0];
struct mem_region *pmap_allocated = &pmap_allocated_regions[0];
int pmap_cnt_avail, pmap_cnt_allocated;
uint32_t pmap_avail_kvo;


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

struct pmapvp2 {
	uint32_t *l2[VP_IDX2_CNT];
	struct pmapvp3 *vp[VP_IDX2_CNT];
};

struct pmapvp3 {
	struct pte_desc *vp[VP_IDX3_CNT];
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
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;
	struct pte_desc *pted;

	vp2 = pm->pm_vp[VP_IDX1(va)];
	if (vp2 == NULL) {
		return NULL;
	}

	vp3 = vp2->vp[VP_IDX2(va)];
	if (vp3 == NULL) {
		return NULL;
	}

	pted = vp3->vp[VP_IDX3(va)];

	return pted;
}

/*
 * Remove, and return, pted at specified address, NULL if not present
 */
struct pte_desc *
pmap_vp_remove(pmap_t pm, vaddr_t va)
{
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;
	struct pte_desc *pted;

	vp2 = pm->pm_vp[VP_IDX1(va)];
	if (vp2 == NULL) {
		return NULL;
	}

	vp3 = vp2->vp[VP_IDX2(va)];
	if (vp3 == NULL) {
		return NULL;
	}

	pted = vp3->vp[VP_IDX3(va)];
	vp3->vp[VP_IDX3(va)] = NULL;

	return pted;
}

const struct kmem_pa_mode kp_l2 = {
	.kp_constraint = &no_constraint,
	.kp_cacheattr = PMAP_CACHE_PTE,
	.kp_maxseg = 1,
	.kp_align = L2_TABLE_SIZE,
	.kp_zero = 1,
};

/*
 * Create a V -> P mapping for the given pmap and virtual address
 * with reference to the pte descriptor that is used to map the page.
 * This code should track allocations of vp table allocations
 * so they can be freed efficiently.
 */
void
pmap_vp_enter(pmap_t pm, vaddr_t va, struct pte_desc *pted)
{
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;
	paddr_t l2_pa = 0;
	vaddr_t l2_va = 0;
	vaddr_t base_va;
	int s;

	vp2 = pm->pm_vp[VP_IDX1(va)];
	if (vp2 == NULL) {
		s = splvm();
		vp2 = pool_get(&pmap_vp_pool, PR_NOWAIT | PR_ZERO);
		splx(s);
		pm->pm_vp[VP_IDX1(va)] = vp2;
	}

	vp3 = vp2->vp[VP_IDX2(va)];
	if (vp3 == NULL) {
		/* No vp3 means no L2 yet, allocate now. */
		s = splvm();
		l2_va = (vaddr_t)pool_get(&pmap_l2_pool, PR_NOWAIT | PR_ZERO);
		splx(s);

		/* XXX: Check error? */
		pmap_extract(pmap_kernel(), l2_va, &l2_pa);

		base_va = va & ~((0x1 << VP_IDX2_POS) - 1);
		pmap_set_l2(pm, base_va, l2_va, l2_pa);

		s = splvm();
		vp3 = pool_get(&pmap_vp_pool, PR_NOWAIT | PR_ZERO);
		splx(s);

		vp2->vp[VP_IDX2(base_va)] = vp3;
	}

	vp3->vp[VP_IDX3(va)] = pted;
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
	return (pted->pted_pte != 0);
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

	//if (!cold) printf("%s: %x %x %x %x %x %x\n", __func__, va, pa, prot, flags, pm, pmap_kernel());

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

	/*
	 * If it should be enabled _right now_, we can skip doing ref/mod
	 * emulation. Any access includes reference, modified only by write.
	 */
	if (pg != NULL &&
	    ((flags & PROT_MASK) || (pg->pg_flags & PG_PMAP_REF))) {
		pg->pg_flags |= PG_PMAP_REF;
		if ((prot & PROT_WRITE) && (flags & PROT_WRITE)) {
			pg->pg_flags |= PG_PMAP_MOD;
		}
	}

	pmap_fill_pte(pm, va, pa, pted, prot, flags, cache);

	if (pg != NULL) {
		pmap_enter_pv(pted, pg); /* only managed mem */
	}

	if (prot & PROT_EXEC) {
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
		if ((prot & PROT_WRITE) && (pg != NULL))
			atomic_clearbits_int(&pg->pg_flags, PG_PMAP_EXE);
	}

	/* only instruction sync executable pages */
	if (need_sync) {
		cache_icache_sync_fresh(va, pa, PAGE_SIZE);
	}

	/*
	 * Insert into table, if this mapping said it needed to be mapped
	 * now.
	 */
	if (flags & (PROT_READ|PROT_WRITE|PROT_EXEC|PMAP_WIRED)) {
		pte_insert(pted);
	}

	ttlb_flush(va & PTE_RPGN);

	splx(s);

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
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;

	//if (!cold) printf("%s: %x %x %x\n", __func__, pm, va, endva);

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
			e_vp2 = VP_IDX2_CNT-1;

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
				e_vp3 = VP_IDX3_CNT;

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

	//if (!cold) printf("%s: %x %x\n", __func__, pm, va);

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

	pte_remove(pted);

	ttlb_flush(pted->pted_va & PTE_RPGN);

	if (pted->pted_va & PTED_VA_EXEC_M) {
		pted->pted_va &= ~PTED_VA_EXEC_M;
	}

	pted->pted_pte = 0;

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

	//if (!cold) printf("%s: %x %x %x %x %x\n", __func__, va, pa, prot, flags, cache);

	pm = pmap_kernel();

	/* MP - lock pmap. */
	s = splvm();

	pted = pmap_vp_lookup(pm, va);

	/* Do not have pted for this, get one and put it in VP */
	if (pted == NULL) {
		panic("pted not preallocated in pmap_kernel() va %lx pa %lx\n",
		    va, pa);
	}

	if (pted && PTED_VALID(pted))
		pmap_kremove_pg(va); /* pted is reused */

	pm->pm_stats.resident_count++;

	/* XXX: multiple pages with different cache attrs? */
	/* XXX: need to write back cache when some page goes WT from WB */
	if (cache == PMAP_CACHE_DEFAULT) {
		/* MAXIMUM cacheability */
		cache = PMAP_CACHE_WB; /* managed memory is cacheable */
	}

	flags |= PMAP_WIRED; /* kernel mappings are always wired. */
	/* Calculate PTE */
	pmap_fill_pte(pm, va, pa, pted, prot, flags, cache);

	/*
	 * Insert into table
	 * We were told to map the page, probably called from vm_fault,
	 * so map the page!
	 */
	pte_insert(pted);

	ttlb_flush(va & PTE_RPGN);

	pm->pm_pt1gen++;

	splx(s);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	_pmap_kenter_pa(va, pa, prot, prot, PMAP_CACHE_DEFAULT);
}

void
pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable)
{
	_pmap_kenter_pa(va, pa, prot, prot, cacheable);
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

	//if (!cold) printf("%s: %x\n", __func__, va);

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
	pte_remove(pted);

	ttlb_flush(pted->pted_va & PTE_RPGN);

	if (pted->pted_va & PTED_VA_EXEC_M)
		pted->pted_va &= ~PTED_VA_EXEC_M;

	if (PTED_MANAGED(pted))
		pmap_remove_pv(pted);

	/* invalidate pted; */
	pted->pted_pte = 0;
	pted->pted_va = 0;

	splx(s);

}

/*
 * remove kernel (pmap_kernel()) mappings
 */
void
pmap_kremove(vaddr_t va, vsize_t len)
{
	//if (!cold) printf("%s: %x %x\n", __func__, va, len);
	for (len >>= PAGE_SHIFT; len >0; len--, va += PAGE_SIZE)
		pmap_kremove_pg(va);

	pmap_kernel()->pm_pt1gen++;
}

void
pte_invalidate(void *ptp, struct pte_desc *pted)
{
	panic("implement pte_invalidate");
}



void
pmap_fill_pte(pmap_t pm, vaddr_t va, paddr_t pa, struct pte_desc *pted,
    vm_prot_t prot, int flags, int cache)
{
	pted->pted_va = va;
	pted->pted_pmap = pm;

	/* NOTE: uses TEX remap */
	switch (cache) {
	case PMAP_CACHE_WB:
		break;
	case PMAP_CACHE_WT:
		break;
	case PMAP_CACHE_CI:
		break;
	case PMAP_CACHE_PTE:
		break;
	default:
		panic("pmap_fill_pte:invalid cache mode");
	}
	pted->pted_va |= cache;

	pted->pted_va |= prot & (PROT_READ|PROT_WRITE|PROT_EXEC);

	if (flags & PMAP_WIRED)
		pted->pted_va |= PTED_VA_WIRED_M;

	pted->pted_pte = pa & PTE_RPGN;
	pted->pted_pte |= flags & (PROT_READ|PROT_WRITE|PROT_EXEC);
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
	//printf("%s\n", __func__);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	/* simple_lock(&pmap_zero_page_lock); */
	pmap_kenter_pa(zero_page, pa, PROT_READ|PROT_WRITE);

	bzero((void *)zero_page, PAGE_SIZE);

	pmap_kremove_pg(zero_page);
}

/*
 * copy the given physical page with zeros.
 */
void
pmap_copy_page(struct vm_page *srcpg, struct vm_page *dstpg)
{
	//printf("%s\n", __func__);
	paddr_t srcpa = VM_PAGE_TO_PHYS(srcpg);
	paddr_t dstpa = VM_PAGE_TO_PHYS(dstpg);
	/* simple_lock(&pmap_copy_page_lock); */

	pmap_kenter_pa(copy_src_page, srcpa, PROT_READ);
	pmap_kenter_pa(copy_dst_page, dstpa, PROT_READ|PROT_WRITE);

	bcopy((void *)copy_src_page, (void *)copy_dst_page, PAGE_SIZE);

	pmap_kremove_pg(copy_src_page);
	pmap_kremove_pg(copy_dst_page);
	/* simple_unlock(&pmap_copy_page_lock); */
}


const struct kmem_pa_mode kp_l1 = {
	.kp_constraint = &no_constraint,
	.kp_cacheattr = PMAP_CACHE_PTE,
	.kp_maxseg = 1,
	.kp_align = L1_TABLE_SIZE,
	.kp_zero = 1,
};

void
pmap_pinit(pmap_t pm)
{
	uint32_t vm_offset, vm_end, vm_size;
	bzero(pm, sizeof (struct pmap));

	/* Allocate a full L1 table. */
	while (pm->pm_pt1 == NULL) {
		pm->pm_pt1 = km_alloc(L1_TABLE_SIZE, &kv_any,
		    &kp_l1, &kd_waitok);
	}

	pmap_extract(pmap_kernel(), (uint32_t)pm->pm_pt1, (paddr_t *)&pm->pm_pt1pa);

	vm_offset = VM_MIN_KERNEL_ADDRESS >> VP_IDX2_POS;
	vm_end = 0xffffffff >> VP_IDX2_POS;
	vm_size = (vm_end - vm_offset + 1) * sizeof(uint32_t);
	memcpy(&pm->pm_pt1[vm_offset], &(pmap_kernel()->pm_pt1[vm_offset]), vm_size);
	dcache_wb_pou((vaddr_t)&pm->pm_pt1[vm_offset], vm_size);

	pm->pm_pt1gen = pmap_kernel()->pm_pt1gen;

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
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;

	for (i = 0; i < VP_IDX1_CNT; i++) {
		vp2 = pm->pm_vp[i];
		if (vp2 == NULL)
			continue;

		for (j = 0; j < VP_IDX2_CNT; j++) {
			vp3 = vp2->vp[j];
			if (vp3 == NULL)
				continue;

			s = splvm();
			pool_put(&pmap_vp_pool, vp3);
			pool_put(&pmap_l2_pool, vp2->l2[j]);
			splx(s);
		}
		pm->pm_vp[i] = NULL;
		s = splvm();
		pool_put(&pmap_vp_pool, vp2);
		splx(s);
	}
}

#if 0
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

		va = (vaddr_t)pa;	/* 1:1 mapping */
		bzero((void *)va, size);
	}

	if (start != NULL)
		*start = VM_MIN_KERNEL_ADDRESS;
	if (end != NULL)
		*end = VM_MAX_KERNEL_ADDRESS;

	return (va);
}
#endif

vaddr_t virtual_avail, virtual_end;

void pmap_setup_avail( uint32_t ram_start, uint32_t ram_end, uint32_t kvo);
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
	void *pa, *va;
	struct pmapvp2 *vp2;
	struct pmapvp3 *vp3;
	struct pte_desc *pted;
	vaddr_t vstart, vend;
	int i, j, k;
	int dump = 0;

	kvo = KERNEL_BASE_VIRT - (ram_start +(KERNEL_BASE_VIRT&0x0fffffff));

	pmap_setup_avail(ram_start, ram_end, kvo);

	/* in theory we could start with just the memory in the kernel,
	 * however this could 'allocate' the bootloader and bootstrap
	 * vm table, which we may need to preserve until later.
	 *   pmap_remove_avail(kernelstart-kvo, kernelend-kvo);
	 */
	pmap_remove_avail(VM_MIN_KERNEL_ADDRESS-kvo, kernelend-kvo);


	/* allocate kernel l1 page table */
	pa = pmap_steal_avail(L1_TABLE_SIZE, L1_TABLE_SIZE, &va);
	pmap_kernel()->pm_pt1 = va;
	pmap_kernel()->pm_pt1pa = (uint32_t)pa;
	pmap_kernel()->pm_pt1gen = 0;

	/* allocate v->p mappings for pmap_kernel() */
	for (i = 0; i < VP_IDX1_CNT; i++) {
		pmap_kernel()->pm_vp[i] = NULL;
	}

	int lb_idx2, ub_idx2;
	int lb_idx3, ub_idx3;

	/*
	 * this loop is done twice, once for large allocations and
	 * once for small
	 */
	for (i = VP_IDX1(VM_MIN_KERNEL_ADDRESS);
	    i <= VP_IDX1(VM_MAX_KERNEL_ADDRESS);
	    i++) {
		pa = pmap_steal_avail(sizeof (struct pmapvp2), 4, &va);
		vp2 = va;
		pmap_kernel()->pm_vp[i] = vp2;

		if (i == VP_IDX1(VM_MIN_KERNEL_ADDRESS)) {
			lb_idx2 = VP_IDX2(VM_MIN_KERNEL_ADDRESS);
		} else {
			lb_idx2 = 0;
		}
		if (i == VP_IDX1(VM_MIN_KERNEL_ADDRESS)) {
			ub_idx2 = VP_IDX2(VM_MAX_KERNEL_ADDRESS);
		} else {
			ub_idx2 = VP_IDX2_CNT-1;
		}
		for (j = lb_idx2; j <= ub_idx2; j++) {
			pa = pmap_steal_avail(sizeof (struct pmapvp3), 4, &va);
			vp3 = va;
			vp2->vp[j] = vp3;
			pa = pmap_steal_avail(L2_TABLE_SIZE, L2_TABLE_SIZE, &va);
			pmap_set_l2(pmap_kernel(),
			    (i << VP_IDX1_POS) | (j << VP_IDX2_POS),
			    (vaddr_t)va, (uint32_t)pa);
		}
	}
	pmap_curmaxkvaddr = VM_MAX_KERNEL_ADDRESS;

	/* allocate pted */
	for (i = VP_IDX1(VM_MIN_KERNEL_ADDRESS);
	    i <= VP_IDX1(VM_MAX_KERNEL_ADDRESS);
	    i++) {
		vp2 = pmap_kernel()->pm_vp[i];

		if (i == VP_IDX1(VM_MIN_KERNEL_ADDRESS)) {
			lb_idx2 = VP_IDX2(VM_MIN_KERNEL_ADDRESS);
		} else {
			lb_idx2 = 0;
		}
		if (i == VP_IDX1(VM_MIN_KERNEL_ADDRESS)) {
			ub_idx2 = VP_IDX2(VM_MAX_KERNEL_ADDRESS);
		} else {
			ub_idx2 = VP_IDX2_CNT-1;
		}
		for (j = lb_idx2; j <= ub_idx2; j++) {
			vp3 = vp2->vp[j];

			if ((i == VP_IDX1(VM_MIN_KERNEL_ADDRESS))
			    && (j == VP_IDX2(VM_MIN_KERNEL_ADDRESS))) {
				lb_idx3 = VP_IDX2(VM_MIN_KERNEL_ADDRESS);
			} else {
				lb_idx3 = 0;
			}
			if ((i == VP_IDX1(VM_MAX_KERNEL_ADDRESS))
			    && (j == VP_IDX2(VM_MAX_KERNEL_ADDRESS))) {
				ub_idx3 = VP_IDX2(VM_MAX_KERNEL_ADDRESS);
			} else {
				ub_idx3 = VP_IDX3_CNT-1;
			}
			for (k = lb_idx3; k <= ub_idx3; k++) {
				pa = pmap_steal_avail(sizeof(struct pte_desc),
				    4, &va);
				pted = va;
				vp3->vp[k] = pted;
			}
		}
	}
	/*
	 * XXX - what about vector table at 0, ever?
	 * XXX - should this be done after mapping space, but before
	 * giving ram to uvm, but the vps have to be mapped, sigh.
	 */
	{
		uint32_t vect = 0xffff0000;
		uint32_t idx1 = VP_IDX1(vect);
		uint32_t idx2 = VP_IDX2(vect);
		uint32_t idx3 = VP_IDX3(vect);

		vp2 = pmap_kernel()->pm_vp[idx1];
		if (vp2 == NULL) {
			pa = pmap_steal_avail(sizeof (struct pmapvp2), 4, &va);
			vp2 = va;
			pmap_kernel()->pm_vp[idx1] = vp2;
printf("had to allocate vp2 %x %x %x %x\n", idx1, idx2, vect, pa);
		}
		vp3 = vp2->vp[idx2];
		if (vp3 == NULL) {
printf("had to allocate vp3 %x %x %x %x\n", idx1, idx2, vect, pa);
			pa = pmap_steal_avail(sizeof (struct pmapvp3), 4, &va);
			vp3 = va;
			vp2->vp[idx2] = vp3;
		}
		pa = pmap_steal_avail(sizeof(struct pte_desc),
		    4, &va);
		pted = va;
printf("allocated pted vp3 %x %x %x %x\n", idx1, idx2, vect, pa);
		vp3->vp[idx3] = pted;

		pa = pmap_steal_avail(L2_TABLE_SIZE, L2_TABLE_SIZE, &va);
		pmap_set_l2(pmap_kernel(), vect,
		    (vaddr_t)va, (uint32_t)pa);
	}

	/* now that we have mapping space for everything, lets map it */
	/* all of these mappings are ram -> kernel va */

#if 0
	struct mem_region *mp;
	vm_prot_t prot;

	for (mp = pmap_allocated; mp->size != 0; mp++) {
		vaddr_t va;
		paddr_t pa;
		vsize_t size;
		extern char *etext;
		printf("mapping %08x sz %08x\n", mp->start, mp->size);
		for (pa = mp->start, va = pa + kvo, size = mp->size & ~0xfff;
		    size > 0; va += PAGE_SIZE, pa+= PAGE_SIZE, size -= PAGE_SIZE) {
			pa = va - kvo;
			prot = PROT_READ|PROT_WRITE;
			if (va >= KERNEL_BASE_VIRT && va < (vaddr_t)etext)
				prot |= PROT_EXEC;
			pmap_kenter_cache(va, pa, prot, PMAP_CACHE_WB);
		}
	}
#endif

	printf("all mapped\n");



	/* XXX */
	printf("stolen 0x%x memory\n", arm_kvm_stolen);
	pmap_avail_fixup();
	vstart = pmap_map_stolen();
	vend = VM_MAX_KERNEL_ADDRESS;

	/* XXX */

	zero_page = vstart;
	vstart += PAGE_SIZE;
	icache_page = vstart;
	vstart += PAGE_SIZE;
	copy_src_page = vstart;
	vstart += PAGE_SIZE;
	copy_dst_page = vstart;
	vstart += PAGE_SIZE;
	msgbufaddr = (caddr_t)vstart;
	vstart += round_page(MSGBUFSIZE);
#if 0
	vstart += reserve_dumppages( (caddr_t)(VM_MIN_KERNEL_ADDRESS +
	    arm_kvm_stolen));
#endif
	/*
	 * Managed KVM space is what we have claimed up to end of
	 * mapped kernel buffers.
	 */
	virtual_avail = vstart;
	virtual_end = vend;


	if (dump) {
	for (i = 0; i < 4096; i++) {
		int idx1, idx2, idx3;
		uint32_t pa = i << VP_IDX2_POS;
		idx1 = pa >> VP_IDX1_POS;
		idx2 = (pa >> VP_IDX2_POS) & VP_IDX2_MASK;
		struct pmapvp3 *vp3;
		uint32_t *l2p;

		if  (pmap_kernel()->pm_pt1[i] != 0) {
			printf("%04x: %08x\n", i, pmap_kernel()->pm_pt1[i]);
			if (pmap_kernel()->pm_vp[idx1] == 0) {
				printf("no tx for idx %d\n", idx1);
				continue;
			}
			l2p = pmap_kernel()->pm_vp[idx1]->l2[idx2];
			vp3 = pmap_kernel()->pm_vp[idx1]->vp[idx2];
			for (idx3 = 0; idx3 < VP_IDX3_CNT; idx3++) {
				if (l2p == 0) {
					printf("no mapping at %d:%d\n", idx1, idx2);
					continue;
				}

				if (l2p[idx3] != 0) {
					printf("%04x %02x: %08x\n", i, idx3,
					    l2p[idx3]);
				}
				if (vp3->vp[idx3] != NULL) {
					if (vp3->vp[idx3]->pted_pte != 0) {
						printf("%04x %02x:"
						    " vp %08x %p va %08x\n", i,
						    idx3,
						    vp3->vp[idx3]->pted_pte,
						    vp3->vp[idx3]->pted_pmap,
						    vp3->vp[idx3]->pted_va
						    );
					}
				}
			}
		}
	}
	}

	uvmexp.pagesize = PAGE_SIZE;
	uvm_setpagesize();

	pmap_physload_avail();

}

void
pmap_set_l2(struct pmap *pm, uint32_t va, vaddr_t l2_va, uint32_t l2_pa)
{
	uint32_t pg_entry;
	struct pmapvp2 *vp2;
	int idx1, idx2;

	if (l2_pa & (L2_TABLE_SIZE-1)) {
		panic("misaligned L2 table\n");
	}

	pg_entry = l2_pa;
	// pg_entry |= pm->domain << L1_S_DOM_POS;
	/* XXX at the current time NS and PXN are never set on L1 */

	pg_entry |= L1_TYPE_PT;

	//idx1 = va >> VP_IDX1_POS;
	//idx2 = (va >> VP_IDX2_POS) & VP_IDX2_MASK;
	idx1 = VP_IDX1(va);
	idx2 = VP_IDX2(va);
	vp2 = pm->pm_vp[idx1];
	vp2->l2[idx2] = (uint32_t *)l2_va;

	pm->pm_pt1[va>>VP_IDX2_POS] = pg_entry;
	__asm __volatile("dsb");
	dcache_wb_pou((vaddr_t)&pm->pm_pt1[va>>VP_IDX2_POS], sizeof(pm->pm_pt1[va>>VP_IDX2_POS]));

	ttlb_flush_range(va & PTE_RPGN, 1<<VP_IDX2_POS);
}

/*
 * activate a pmap entry
 */
void
pmap_activate(struct proc *p)
{
	pmap_t pm;
	struct pcb *pcb;
	intr_state_t its;
	uint32_t vm_offset, vm_end, vm_size;

	its = intr_disable();

	pm = p->p_vmspace->vm_map.pmap;
	pcb = &p->p_addr->u_pcb;

	if (pm->pm_pt1gen != pmap_kernel()->pm_pt1gen) {
		vm_offset = VM_MIN_KERNEL_ADDRESS >> VP_IDX2_POS;
		vm_end = 0xffffffff >> VP_IDX2_POS;
		vm_size = (vm_end - vm_offset + 1) * sizeof(uint32_t);
		memcpy(&pm->pm_pt1[vm_offset], &(pmap_kernel()->pm_pt1[vm_offset]), vm_size);
		dcache_wb_pou((vaddr_t)&pm->pm_pt1[vm_offset], vm_size);
		pm->pm_pt1gen = pmap_kernel()->pm_pt1gen;
	}

	pcb->pcb_pl1vec = NULL;
	pcb->pcb_pagedir = pm->pm_pt1pa | ttb_flags;

	if (p == curproc) {
		cpu_setttb(pcb->pcb_pagedir);
	}

	intr_restore(its);
}

/*
 * deactivate a pmap entry
 */
void
pmap_deactivate(struct proc *p)
{
	/* NOOP */
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

	if (pted->pted_pte == 0)
		return FALSE;

	if (pa != NULL)
		*pa = (pted->pted_pte & PTE_RPGN) | (va & ~PTE_RPGN);

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
	struct pte_desc *pted;

	/* Every VA needs a pted, even unmanaged ones. */
	pted = pmap_vp_lookup(pm, va);
	if (!pted || !PTED_VALID(pted)) {
		return;
	}

	pted->pted_va &= ~PROT_WRITE;
	pted->pted_pte &= ~PROT_WRITE;
	pte_insert(pted);

	ttlb_flush(pted->pted_va & PTE_RPGN);

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

	//if (!cold) printf("%s: prot %x\n", __func__, prot);

	/* need to lock for this pv */
	s = splvm();

	if (prot == PROT_NONE) {
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
	//if (!cold) printf("%s\n", __func__);

	int s;
	if (prot & (PROT_READ | PROT_EXEC)) {
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
	pool_init(&pmap_vp_pool, sizeof(struct pmapvp2), 0, 0, 0, "vp", NULL);
	pool_setlowat(&pmap_vp_pool, 10);
	pool_init(&pmap_pted_pool, sizeof(struct pte_desc), 0, 0, 0, "pted",
	    NULL);
	pool_setlowat(&pmap_pted_pool, 20);
	pool_init(&pmap_l2_pool, L2_TABLE_SIZE, L2_TABLE_SIZE, 0, 0, "l2",
	    NULL);
	pool_setlowat(&pmap_l2_pool, 10);

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

static uint32_t ap_bits_user [8] = {
	[PROT_NONE]				= 0,
	[PROT_READ]				= L2_P_AP2|L2_P_AP1|L2_P_AP0|L2_P_XN,
	[PROT_WRITE]				= L2_P_AP1|L2_P_AP0|L2_P_XN,
	[PROT_WRITE|PROT_READ]			= L2_P_AP1|L2_P_AP0|L2_P_XN,
	[PROT_EXEC]				= L2_P_AP2|L2_P_AP1|L2_P_AP0,
	[PROT_EXEC|PROT_READ]			= L2_P_AP2|L2_P_AP1|L2_P_AP0,
	[PROT_EXEC|PROT_WRITE]			= L2_P_AP1|L2_P_AP0,
	[PROT_EXEC|PROT_WRITE|PROT_READ]	= L2_P_AP1|L2_P_AP0,
};

static uint32_t ap_bits_kern [8] = {
	[PROT_NONE]				= 0,
	[PROT_READ]				= L2_P_AP2|L2_P_AP0|L2_P_XN,
	[PROT_WRITE]				= L2_P_AP0|L2_P_XN,
	[PROT_WRITE|PROT_READ]			= L2_P_AP0|L2_P_XN,
	[PROT_EXEC]				= L2_P_AP2|L2_P_AP0,
	[PROT_EXEC|PROT_READ]			= L2_P_AP2|L2_P_AP0,
	[PROT_EXEC|PROT_WRITE]			= L2_P_AP0,
	[PROT_EXEC|PROT_WRITE|PROT_READ]	= L2_P_AP0,
};

void
pte_insert(struct pte_desc *pted)
{
	/* put entry into table */
	/* need to deal with ref/change here */
	uint32_t pte, cache_bits, access_bits;
	struct pmapvp2 *vp2;
	uint32_t *l2;
	pmap_t pm = pted->pted_pmap;

	/* NOTE: uses TEX remap */
	switch (pted->pted_va & PMAP_CACHE_BITS) {
	case PMAP_CACHE_WB:
		cache_bits = L2_MODE_MEMORY;
		break;
	case PMAP_CACHE_WT: /* for the momemnt treating this as uncached */
		cache_bits = L2_MODE_DISPLAY;
		break;
	case PMAP_CACHE_CI:
		cache_bits = L2_MODE_DEV;
		break;
	case PMAP_CACHE_PTE:
		cache_bits = L2_MODE_PTE;
		break;
	default:
		panic("pte_insert:invalid cache mode");
	}

	if (pm == pmap_kernel()) {
		access_bits = ap_bits_kern[pted->pted_pte & PROT_MASK];
	} else {
		access_bits = ap_bits_user[pted->pted_pte & PROT_MASK];
	}

	if (access_bits == 0)
		pte = 0;
	else
		pte = (pted->pted_pte & PTE_RPGN) | cache_bits |
		    access_bits | L2_P;

	vp2 = pm->pm_vp[VP_IDX1(pted->pted_va)];
	if (vp2->l2[VP_IDX2(pted->pted_va)] == NULL) {
		panic("have a pted, but missing the l2 for %x va pmap %x",
		    pted->pted_va, pm);
	}
	l2 = vp2->l2[VP_IDX2(pted->pted_va)];
	l2[VP_IDX3(pted->pted_va)] = pte;
	__asm __volatile("dsb");
	dcache_wb_pou((vaddr_t)&l2[VP_IDX3(pted->pted_va)], sizeof(l2[VP_IDX3(pted->pted_va)]));
	//cpu_tlb_flushID_SE(pted->pted_va & PTE_RPGN);
}

void
pte_remove(struct pte_desc *pted)
{
	/* put entry into table */
	/* need to deal with ref/change here */
	struct pmapvp2 *vp2;
	uint32_t *l2;
	pmap_t pm = pted->pted_pmap;

	vp2 = pm->pm_vp[VP_IDX1(pted->pted_va)];
	if (vp2->l2[VP_IDX2(pted->pted_va)] == NULL) {
		panic("have a pted, but missing the l2 for %x va pmap %x",
		    pted->pted_va, pm);
	}
	l2 = vp2->l2[VP_IDX2(pted->pted_va)];
	l2[VP_IDX3(pted->pted_va)] = 0;
	__asm __volatile("dsb");
	dcache_wb_pou((vaddr_t)&l2[VP_IDX3(pted->pted_va)], sizeof(l2[VP_IDX3(pted->pted_va)]));
	//cpu_tlb_flushID_SE(pted->pted_va & PTE_RPGN);
}

/*
 * This function exists to do software referenced/modified emulation.
 * It's purpose is to tell the caller that a fault was generated either
 * for this emulation, or to tell the caller that it's a legit fault.
 */
int pmap_fault_fixup(pmap_t pm, vaddr_t va, vm_prot_t ftype, int user)
{
	struct pte_desc *pted;
	struct vm_page *pg;
	paddr_t pa;

	//printf("fault pm %x va %x ftype %x user %x\n", pm, va, ftype, user);

	/* Every VA needs a pted, even unmanaged ones. */
	pted = pmap_vp_lookup(pm, va);
	if (!pted || !PTED_VALID(pted)) {
		return 0;
	}

	/* There has to be a PA for the VA, go look. */
	if (pmap_extract(pm, va, &pa) == FALSE) {
		return 0;
	}

	/* If it's unmanaged, it must not fault. */
	pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL) {
		return 0;
	}

	/*
	 * Check the fault types to find out if we were doing
	 * any mod/ref emulation and fixup the PTE if we were.
	 */
	if ((ftype & PROT_WRITE) && /* fault caused by a write */
	    !(pted->pted_pte & PROT_WRITE) && /* and write is disabled now */
	    (pted->pted_va & PROT_WRITE)) { /* but is supposedly allowed */

		/*
		 * Page modified emulation. A write always
		 * includes a reference.
		 */
		pg->pg_flags |= PG_PMAP_MOD;
		pg->pg_flags |= PG_PMAP_REF;

		/* Thus, enable read and write. */
		pted->pted_pte |= (pted->pted_va & (PROT_READ|PROT_WRITE));

		/* Insert change. */
		pte_insert(pted);

		/* Flush tlb. */
		ttlb_flush(va & PTE_RPGN);

		return 1;
	} else if ((ftype & PROT_EXEC) && /* fault caused by an exec */
	    !(pted->pted_pte & PROT_EXEC) && /* and exec is disabled now */
	    (pted->pted_va & PROT_EXEC)) { /* but is supposedly allowed */

		/*
		 * Exec always includes a read/reference.
		 */
		pg->pg_flags |= PG_PMAP_REF;

		/* Thus, enable read and exec. */
		pted->pted_pte |= (pted->pted_va & (PROT_READ|PROT_EXEC));

		/* Insert change. */
		pte_insert(pted);

		/* Flush tlb. */
		ttlb_flush(va & PTE_RPGN);

		return 1;
	} else if ((ftype & PROT_READ) && /* fault caused by a read */
	    !(pted->pted_pte & PROT_READ) && /* and read is disabled now */
	    (pted->pted_va & PROT_READ)) { /* but is supposedly allowed */

		/*
		 * Page referenced emulation.
		 */
		pg->pg_flags |= PG_PMAP_REF;

		/* Thus, enable read. */
		pted->pted_pte |= (pted->pted_va & PROT_READ);

		/* Insert change. */
		pte_insert(pted);

		/* Flush tlb. */
		ttlb_flush(va & PTE_RPGN);

		return 1;
	}

	/* didn't catch it, so probably broken */
	return 0;
}

void pmap_postinit(void) {}
void    pmap_map_section(vaddr_t l1_addr, vaddr_t va, paddr_t pa, int flags, int cache) {
	uint32_t *l1 = (uint32_t *)l1_addr;
	uint32_t cache_bits;
	int ap_flag;

	switch (flags) {
	case PROT_READ:
		ap_flag = L1_S_AP2|L1_S_AP0|L1_S_XN;
		break;
	case PROT_READ | PROT_WRITE:
		ap_flag = L1_S_AP0|L1_S_XN;
		break;
	}

	switch (cache) {
	case PMAP_CACHE_WB:
		cache_bits = L1_MODE_MEMORY;
		break;
	case PMAP_CACHE_WT: /* for the momemnt treating this as uncached */
		cache_bits = L1_MODE_DISPLAY;
		break;
	case PMAP_CACHE_CI:
		cache_bits = L1_MODE_DEV;
		break;
	case PMAP_CACHE_PTE:
		cache_bits = L1_MODE_PTE;
		break;
	}

	l1[va>>VP_IDX2_POS] = (pa & L1_S_RPGN) | ap_flag | cache_bits | L1_TYPE_S;
}

void    pmap_map_entry(vaddr_t l1, vaddr_t va, paddr_t pa, int i0, int i1) {}
vsize_t pmap_map_chunk(vaddr_t l1, vaddr_t va, paddr_t pa, vsize_t sz, int prot, int cache)
{
	for (; sz > 0; sz -= PAGE_SIZE, va += PAGE_SIZE, pa += PAGE_SIZE) {
		pmap_kenter_cache(va, pa, prot, cache);
	}
	return 0;
}


void pmap_update()
{
}

void memhook(void);

void memhook()
{
}

paddr_t zero_page;
paddr_t icache_page;
paddr_t copy_src_page;
paddr_t copy_dst_page;


int pmap_is_referenced(struct vm_page *pg)
{
	//printf("%s\n", __func__);
	return ((pg->pg_flags & PG_PMAP_REF) != 0);
}

int pmap_is_modified(struct vm_page *pg)
{
	//printf("%s\n", __func__);
	return ((pg->pg_flags & PG_PMAP_MOD) != 0);
}

int pmap_clear_modify(struct vm_page *pg)
{
	struct pte_desc *pted;

	//printf("%s\n", __func__);

	pg->pg_flags &= ~PG_PMAP_MOD;

	LIST_FOREACH(pted, &(pg->mdpage.pv_list), pted_pv_list) {
		pte_remove(pted);
		pted->pted_pte &= ~PROT_WRITE;
		pte_insert(pted);
	}

	return 0;
}

int pmap_clear_reference(struct vm_page *pg)
{
	struct pte_desc *pted;

	//printf("%s\n", __func__);

	pg->pg_flags &= ~PG_PMAP_REF;

	LIST_FOREACH(pted, &(pg->mdpage.pv_list), pted_pv_list) {
		pte_remove(pted);
		pted->pted_pte &= ~PROT_MASK;
	}

	return 0;
}

void pmap_copy(pmap_t src_pmap, pmap_t dst_pmap, vaddr_t src, vsize_t sz, vaddr_t dst)
{
	//printf("%s\n", __func__);
}

void pmap_unwire(pmap_t pm, vaddr_t va)
{
	struct pte_desc *pted;

	//printf("%s\n", __func__);

	pted = pmap_vp_lookup(pm, va);
	if (pted != NULL)
		pted->pted_va &= ~PTED_VA_WIRED_M;
}

void pmap_remove_holes(struct vmspace *vm)
{
	/* NOOP */
}

void pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{
	*start = virtual_avail;
	*end = virtual_end;
}

vaddr_t  pmap_curmaxkvaddr;

void pmap_set_pcb_pagedir(pmap_t pm, struct pcb *pcb)
{
	pcb->pcb_pagedir = pm->pm_pt1pa | ttb_flags;
}

void pmap_avail_fixup(void);

void
pmap_setup_avail( uint32_t ram_start, uint32_t ram_end, uint32_t kvo)
{
	/* This makes several assumptions
	 * 1) kernel will be located 'low' in memory
	 * 2) memory will not start at VM_MIN_KERNEL_ADDRESS
	 * 3) several MB of memory starting just after the kernel will
	 *    be premapped at the kernel address in the bootstrap mappings
	 * 4) kvo will be the 32 bit number to add to the ram address to
	 *    obtain the kernel virtual mapping of the ram. eg if ram is at
	 *    0x80000000, and VM_MIN_KERNEL_ADDRESS is 0xc0000000, the kvo
	 *    will be 0x40000000
	 * 5) it is generally assumed that these translations will occur with
	 *    large granularity, at minimum the translation will be page
	 *    aligned, if not 'section' or greater.
	 */

	pmap_avail_kvo = kvo;
	pmap_avail[0].start = ram_start;
	pmap_avail[0].size = ram_end-ram_start;

	/* XXX - multiple sections */
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

/* XXX - this zeros pages via their physical address */
void *
pmap_steal_avail(size_t size, int align, void **kva)
{
	struct mem_region *mp;
	int start;
	int remsize;
	arm_kvm_stolen += size; // debug only

	for (mp = pmap_avail; mp->size; mp++) {
		if (mp->size > size) {
			start = (mp->start + (align -1)) & ~(align -1);
			remsize = mp->size - (start - mp->start);
			if (remsize >= 0) {
				pmap_remove_avail(start, start+size);
				if (kva != NULL){
					*kva = (void *)(start + pmap_avail_kvo);
				}
				bzero((void*)(start+pmap_avail_kvo), size);
				return (void *)start;
			}
		}
	}
	panic ("unable to allocate region with size %x align %x",
	    size, align);
}

vaddr_t
pmap_map_stolen()
{
	int prot;
	struct mem_region *mp;
	uint32_t pa, va, e;
	extern char *etext;


	int oldprot = 0;
	printf("mapping self\n");
	for (mp = pmap_allocated; mp->size; mp++) {
		printf("start %08x end %08x\n", mp->start, mp->start + mp->size);
		printf("exe range %08x, %08x\n", KERNEL_BASE_VIRT,
		    (uint32_t)&etext);
		for (e = 0; e < mp->size; e += PAGE_SIZE) {
			/* XXX - is this a kernel text mapping? */
			/* XXX - Do we care about KDB ? */
			pa = mp->start + e;
			va = pmap_avail_kvo + pa;
			if (va >= KERNEL_BASE_VIRT && va < (uint32_t)&etext) {
				prot = PROT_READ|PROT_WRITE|
				    PROT_EXEC;
			} else {
				prot = PROT_READ|PROT_WRITE;
			}
			if (prot != oldprot) {
				printf("mapping  v %08x p %08x prot %x\n", va,
				    pa, prot);
				oldprot = prot;
			}
			pmap_kenter_cache(va, pa, prot, PMAP_CACHE_WB);
		}
	}
	printf("last mapping  v %08x p %08x\n", va, pa);
	return va + PAGE_SIZE;
}

void
pmap_physload_avail(void)
{
	struct mem_region *mp;
	uint32_t start, end;

	for (mp = pmap_avail; mp->size; mp++) {
		printf("start %08x size %08x", mp->start, mp->size);
		if (mp->size < PAGE_SIZE) {
			printf(" skipped - too small\n");
			continue;
		}
		start = mp->start;
		if (start & PAGE_MASK) {
			start = PAGE_SIZE + (start & ~PAGE_MASK);
		}
		end = mp->start + mp->size;
		if (end & PAGE_MASK) {
			end = (end & ~PAGE_MASK);
		}
		uvm_page_physload(atop(start), atop(end),
		    atop(start), atop(end), 0);

		printf(" loaded %08x %08x\n", start, end);
	}
}
