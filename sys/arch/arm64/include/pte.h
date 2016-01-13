/*
 * Copyright (c) 2014 Dale Rahn <drahn@dalerahn.com>
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
#ifndef _ARM_PTE_H_
#define _ARM_PTE_H_

#include "arm64/vmparam.h"

/*  level X descriptor */
#define	Lx_TYPE_MASK	(0x00000003)	/* mask of type bits */
#define Lx_TYPE_S 	(0x00000001)
#define Lx_TYPE_PT 	(0x00000003)
// XXX need to investigate use of these
#define Lx_PT_NS	(1ULL<<63)
#define Lx_PT_AP00	(0ULL<<61)
#define Lx_PT_AP01	(1ULL<<61)
#define Lx_PT_AP10	(2ULL<<61)
#define Lx_PT_AP11	(3ULL<<61)
#define Lx_PT_XN	(1ULL<<60)
#define Lx_PT_PXN	(1ULL<<59)
#define	Lx_TABLE_ALIGN	(4096)

/* Block and Page attributes */
/* TODO: Add the upper attributes */
#define		ATTR_MASK_H	(0xfff0000000000000ULL)
#define		ATTR_MASK_L	(0x0000000000000fffULL)
#define		ATTR_MASK	(ATTR_MASK_H | ATTR_MASK_L)
/* Bits 58:55 are reserved for software */
#define		ATTR_SW_MANAGED	(1UL << 56)
#define		ATTR_SW_WIRED	(1UL << 55)
#define		ATTR_UXN	(1UL << 54)
#define		ATTR_PXN	(1UL << 53)
#define		ATTR_nG		(1 << 11)
#define		ATTR_AF		(1 << 10)
#define		ATTR_SH(x)	((x) << 8)
#define		ATTR_AP_RW_BIT	(1 << 7)
#define		ATTR_AP(x)	((x) << 6)
#define		ATTR_AP_MASK	ATTR_AP(3)
#define		ATTR_NS		(1 << 5)
#define		ATTR_IDX(x)	((x) << 2)
#define		ATTR_IDX_MASK	(7 << 2)



/* Level 0 table, 512GiB per entry */
#define		L0_SHIFT	39
#define		L0_INVAL	0x0 /* An invalid address */
#define		L0_BLOCK	0x1 /* A block */
	/* 0x2 also marks an invalid address */
#define		L0_TABLE	0x3 /* A next-level table */


/* Level 1 table, 1GiB per entry */
#define		L1_SHIFT	30
#define		L1_SIZE		(1 << L1_SHIFT)
#define		L1_OFFSET	(L1_SIZE - 1)
#define		L1_INVAL	L0_INVAL
#define		L1_BLOCK	L0_BLOCK
#define		L1_TABLE	L0_TABLE
 
/* Level 2 table, 2MiB per entry */
#define		L2_SHIFT	21
#define		L2_SIZE		(1 << L2_SHIFT)
//#define	L2_OFFSET	L2_SIZE - 1)
//#define	L2_INVAL	L0_INVAL
#define		L2_BLOCK	L0_BLOCK
//#define	L2_TABLE	L0_TABLE

#define		L2_SHIFT		21
#define		L2_SIZE		(1 << L2_SHIFT)

// page mapping
#define		L3_P		(3)

#define		Ln_ENTRIES		(1 << 9)
#define		Ln_ADDR_MASK	(Ln_ENTRIES - 1)
#define		Ln_TABLE_MASK	((1 << 12) - 1)

/* Short-descriptor translation table Second level lg Page descriptor format */
#define L2_S_XN		(1<<15)	/* eXecute Never */
#define L2_S_B		(1<<2)	/* bufferable Section */
#define L2_S_C		(1<<3)	/* cacheable Section */
#define L2_S_S		(1<<10)	/* shareable Section */
#define L2_S_nG		(1<<11)	/* notGlobal */
#define L2_S_AP(ap)	((((ap) & 0x4) << 9) | (((ap) & 3) << 4))
#define L2_S_AP0	(1<<4)
#define L2_S_AP1	(1<<5)
#define L2_S_AP2	(1<<9)
#define L2_S_TEX(x)	(((x)&0x7)<<12)

#define L2_S_MODE_DEV		(L2_S_TEX(0)|0|0)
#define L2_S_MODE_PTE		(L2_S_TEX(0)|0|L2_S_B)
#define L2_S_MODE_DISPLAY		(L2_S_TEX(0)|L2_S_C|0)
#define L2_S_MODE_MEMORY		(L2_S_TEX(0)|L2_S_C|L2_S_B)

/* Short-descriptor translation table Second level sm page descriptor format */
#define L2_P_XN		(1<<0)	/* eXecute Never */
#define L2_P 		(1<<1)	/* small page mapping */
#define L2_P_B		(1<<2)	/* bufferable Section */
#define L2_P_C		(1<<3)	/* cacheable Section */
#define L2_P_S		(1<<10)	/* shareable Section */
#define L2_P_nG		(1<<11)	/* notGlobal */
#define L2_P_AP(ap)	((((ap) & 0x4) << 9) | (((ap) & 3) << 4))
#define L2_P_AP0	(1<<4)
#define L2_P_AP1	(1<<5)
#define L2_P_AP2	(1<<9)
#define L2_P_TEX(x)	(((x)&0x7)<<6)

#define L2_MODE_DEV		(L2_P_TEX(0)|0|0)
#define L2_MODE_PTE		(L2_P_TEX(0)|0|L2_S_B)
#define L2_MODE_DISPLAY		(L2_P_TEX(0)|L2_S_C|0)
#define L2_MODE_MEMORY		(L2_P_TEX(0)|L2_S_C|L2_S_B)

/* This defaults to 
 *		INNER	OUTER
 * MODE dev	NC,	NC
 * MODE pte	NC,	WBna
 * MODE display	NC,	NC
 * MODE memory	WBna,	WBna
 * 
 * If the CPU supports WT,
 * DISPLAY should be outer WT (XXX inner WT?, but very small cache?)
 * and PTE inner WT
 * ALSO: all WB mappings here are WriteBack No Allocate, main memory
 * mappings likely should be configured as Write Allocate if supported.
 */
#define NMRR_DEFAULT	0x00cc00c0
#define PRRR_DEFAULT	0xf00a00a9

/* 
 * NON-TEX Access permission bits
 */
#define AP_PF		0x00	/* all accesses generate permission faul
ts */
#define AP_KR		0x05	/* kernel read */
#define AP_KRW		0x01	/* kernel read/write */
#define AP_KRWUR	0x02	/* kernel read/write usr read */
#define AP_KRWURW	0x03	/* kernel read/write usr read/write */

/*
 * Domain Types for the Domain Access Control Register.
 */
#define	DOMAIN_FAULT	0x00		/* no access */
#define	DOMAIN_CLIENT	0x01		/* client */
#define	DOMAIN_RESERVED	0x02		/* reserved */
#define	DOMAIN_MANAGER	0x03		/* manager */

#define PMAP_DOMAIN_KERNEL	0	/* The kernel uses domain #0 */

/* physical page mask */
#define PTE_RPGN 0x3ffffff000ULL

/* XXX */
#ifndef _LOCORE
struct pte {
	uint64_t pte;
};

typedef uint64_t pd_entry_t;	/* L1 table entry */
typedef uint64_t pt_entry_t;	/* L2 table entry */

struct pv_node {
};

#endif /* _LOCORE */


/// REWRITE
#define L2_L_SIZE		0x00010000	/* 64K */
#define L2_L_OFFSET		(L2_L_SIZE - 1)
#define L2_L_FRAME		(~L2_L_OFFSET)
#define L2_L_SHIFT		16

#define L2_S_SIZE		0x00001000	/* 4K */
#define L2_S_OFFSET		(L2_S_SIZE - 1)
#define L2_S_FRAME		(~L2_S_OFFSET)
#define L2_S_SHIFT		12

///

#endif /* _ARM_PTE_H_ */
