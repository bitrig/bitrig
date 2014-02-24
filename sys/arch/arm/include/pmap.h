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
#ifndef	_ARM_PMAP_H_
#define	_ARM_PMAP_H_

#include <arm/pte.h>
#ifndef _LOCORE
#include <arm/cpufunc.h>
#endif

typedef struct pmap *pmap_t;

/* V->P mapping data */
#define VP_IDX1_CNT	32
#define VP_IDX1_MASK	(VP_IDX1_CNT-1)
#define VP_IDX1_POS	28
#define VP_IDX2_CNT	256
#define VP_IDX2_MASK	(VP_IDX2_CNT-1)
#define VP_IDX2_POS	20
#define VP_IDX3_CNT	256
#define VP_IDX3_MASK	(VP_IDX3_CNT-1)
#define VP_IDX3_POS	12

void pmap_kenter_cache(vaddr_t va, paddr_t pa, vm_prot_t prot, int cacheable);
/* cache flags */
#define PMAP_CACHE_DEFAULT	0	/* WB cache managed mem, devices not */
#define PMAP_CACHE_CI		1	/* cache inhibit */
#define PMAP_CACHE_WT		2	/* writethru */
#define PMAP_CACHE_WB		3	/* writeback */

#define PG_PMAP_MOD     PG_PMAP0
#define PG_PMAP_REF     PG_PMAP1
#define PG_PMAP_EXE     PG_PMAP2
 
extern paddr_t zero_page;
extern paddr_t copy_src_page;
extern paddr_t copy_dst_page;

/*
 * Pmap stuff
 */
struct pmap {
	struct pmapvp2 *pm_vp[VP_IDX1_CNT];	/* virtual to physical table */
	uint32_t *l1_va;
	int pm_refs;				/* ref count */
	struct pmap_statistics  pm_stats;	/* pmap statistics */
	/* delete this */ union pmap_cache_state	pm_cstate;
};

#define PTED_VA_MANAGED_M       0x01
#define PTED_VA_WIRED_M         0x02
#define PTED_VA_EXEC_M          0x04

extern struct pmap kernel_pmap_;
#define pmap_kernel()   		(&kernel_pmap_)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

void pmap_bootstrap(u_int kernelstart, u_int kernelend, uint32_t ram_start,
    uint32_t ram_end);
void pmap_page_ro(pmap_t pm, vaddr_t va, vm_prot_t prot);

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
void	pmap_set_pcb_pagedir(pmap_t, struct pcb *);

#define PTE_NOCACHE 0
#define PTE_CACHE 1
#define PTE_PAGETABLE 2

#define KERNEL_BASE (0xc0000000)

vsize_t      pmap_map_chunk(vaddr_t, vaddr_t, paddr_t, vsize_t, int, int);
void	pmap_link_l2pt(vaddr_t, vaddr_t, pv_addr_t *);

extern vaddr_t	pmap_curmaxkvaddr;

#ifndef _LOCORE
/*
 * pmap-specific data store in the vm_page structure.
 */
struct vm_page_md {
	struct pv_entry *pvh_list;		/* pv_entry list */
	struct simplelock pvh_slock;		/* lock on this head */
	int pvh_attrs;				/* page attributes */
	u_int uro_mappings;
	u_int urw_mappings;
	union {
		u_short s_mappings[2];	/* Assume kernel count <= 65535 */
		u_int i_mappings;
	} k_u;
#define	kro_mappings	k_u.s_mappings[0]
#define	krw_mappings	k_u.s_mappings[1]
#define	k_mappings	k_u.i_mappings
};

#define	VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.pvh_list = NULL;					\
	simple_lock_init(&(pg)->mdpage.pvh_slock);			\
	(pg)->mdpage.pvh_attrs = 0;					\
	(pg)->mdpage.uro_mappings = 0;					\
	(pg)->mdpage.urw_mappings = 0;					\
	(pg)->mdpage.k_mappings = 0;					\
} while (/*CONSTCOND*/0)
#endif /* _LOCORE */

#endif	/* _ARM_PMAP_H_ */
