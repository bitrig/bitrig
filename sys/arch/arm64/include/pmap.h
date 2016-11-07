/*
 * Copyright (c) 2008,2009,2014 Dale Rahn <drahn@dalerahn.com>
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
#ifndef	_ARM64_PMAP_H_
#define	_ARM64_PMAP_H_

#include <arm64/pte.h>

#define PMAP_PA_MASK  ~((paddr_t)PAGE_MASK) /* to remove the flags */

typedef struct pmap *pmap_t;

/* V->P mapping data */
#define VP_IDX0_CNT	512
#define VP_IDX0_MASK	(VP_IDX0_CNT-1)
#define VP_IDX0_POS	39
#define VP_IDX1_CNT	512
#define VP_IDX1_MASK	(VP_IDX1_CNT-1)
#define VP_IDX1_POS	30
#define VP_IDX2_CNT	512
#define VP_IDX2_MASK	(VP_IDX2_CNT-1)
#define VP_IDX2_POS	21
#define VP_IDX3_CNT	512
#define VP_IDX3_MASK	(VP_IDX3_CNT-1)
#define VP_IDX3_POS	12

void pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable);
/* cache flags */
#define PMAP_CACHE_DEFAULT	0	/* WB cache managed mem, devices not */
#define PMAP_CACHE_CI		(PMAP_MD0)		/* cache inhibit */
#define PMAP_CACHE_WT		(PMAP_MD1)	 	/* writethru */
#define PMAP_CACHE_WB		(PMAP_MD1|PMAP_MD0)	/* writeback */
#define PMAP_CACHE_PTE		(PMAP_MD2)	/* PTE mapping */
#define PMAP_CACHE_BITS		(PMAP_MD0|PMAP_MD1|PMAP_MD2)	

#define PTED_VA_MANAGED_M	(PMAP_MD3)
#define PTED_VA_WIRED_M		(PMAP_MD3 << 1)
#define PTED_VA_EXEC_M		(PMAP_MD3 << 2)

#define PG_PMAP_MOD		PG_PMAP0
#define PG_PMAP_REF		PG_PMAP1
#define PG_PMAP_EXE		PG_PMAP2

// [NCPUS]
extern paddr_t zero_page;
extern paddr_t copy_src_page;
extern paddr_t copy_dst_page;

/*
 * Pmap stuff
 */
struct pmap {
	union {
		struct pmapvp0 *l0;	/* virtual to physical table 4 lvl */
		struct pmapvp1 *l1;	/* virtual to physical table 3 lvl */
	} pm_vp;
	uint64_t pm_pt0pa;
	int have_4_level_pt;
	int pm_asid;
	int pm_active;
	int pm_refs;				/* ref count */
	struct pmap_statistics  pm_stats;	/* pmap statistics */
};


extern struct pmap kernel_pmap_;
#define pmap_kernel()   		(&kernel_pmap_)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

vaddr_t pmap_bootstrap(long kvo, paddr_t lpt1,  long kernelstart,
    long kernelend, long ram_start, long ram_end);
void pmap_page_ro(pmap_t pm, vaddr_t va, vm_prot_t prot);

void pmap_avail_fixup();
void pmap_physload_avail();

struct pv_entry;

/* investigate */
#define pmap_unuse_final(p)		do { /* nothing */ } while (0)
int	pmap_fault_fixup(pmap_t, vaddr_t, vm_prot_t, int);
void pmap_postinit(void);
void	pmap_map_section(vaddr_t, vaddr_t, paddr_t, int, int);
void	pmap_map_entry(vaddr_t, vaddr_t, paddr_t, int, int);
vsize_t	pmap_map_chunk(vaddr_t, vaddr_t, paddr_t, vsize_t, int, int);

/*
 * Physical / virtual address structure. In a number of places (particularly
 * during bootstrapping) we need to keep track of the physical and virtual
 * addresses of various pages
 */
typedef struct pv_addr {
	SLIST_ENTRY(pv_addr) pv_list;
	paddr_t pv_pa;
	vaddr_t pv_va;
} pv_addr_t;
void	vector_page_setprot(int);

#define PTE_NOCACHE 0
#define PTE_CACHE 1
#define PTE_PAGETABLE 2

vsize_t      pmap_map_chunk(vaddr_t, vaddr_t, paddr_t, vsize_t, int, int);
void	pmap_link_l2pt(vaddr_t, vaddr_t, pv_addr_t *);

extern vaddr_t	pmap_curmaxkvaddr;

#ifndef _LOCORE

#define __HAVE_VM_PAGE_MD
struct vm_page_md {
	LIST_HEAD(,pte_desc) pv_list;
	int pvh_attrs;				/* page attributes */
};

#define VM_MDPAGE_INIT(pg) do {			\
        LIST_INIT(&((pg)->mdpage.pv_list));     \
	(pg)->mdpage.pvh_attrs = 0;		\
} while (0)

#endif /* _LOCORE */

#endif	/* _ARM64_PMAP_H_ */
