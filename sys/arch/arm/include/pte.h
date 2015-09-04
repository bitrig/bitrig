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

#include "arm/vmparam.h"

/* first level descriptor */
#define	L1_TYPE_MASK	(0x00000003)	/* mask of type bits */
#define L1_TYPE_S 	(0x00000002)
#define L1_TYPE_PT 	(0x00000001)
#define L1_PT_DOM_POS	5
#define L1_PT_NS	(1<<3)
#define L1_PT_PXN	(1<<2)

/* Short-decriptor translation table First level descriptor format */
#define L1_S_PXN	(1<<0)	/* privilege eXecute Never */
#define L1_S_B		(1<<2)	/* bufferable Section */
#define L1_S_C		(1<<3)	/* cacheable Section */
#define L1_S_XN		(1<<4)	/* eXecute Never */
#define L1_S_S		(1<<10)	/* shareable Section */
#define L1_S_AP(ap)	((((ap) & 0x4) << 13) | (((ap) & 3) << 10))
#define L1_S_AP0	(1<<10)
#define L1_S_AP1	(1<<11)
#define L1_S_AP2	(1<<15)
#define L1_S_TEX(x)	(((x)&0x7)<<12)
#define L1_S_G(x)	(((x)&0x1)<<17)
#define L1_S_RPGN	(0xfff00000)
#define L1_S_SHIFT	(20)
#define	L1_S_SIZE	0x00100000	/* 1M */
#define	L1_S_OFFSET	(L1_S_SIZE-1)	/* 1M */
#define L1_TABLE_SIZE	(16 * 1024)

/* TEX remap modes */
#define L1_MODE_DEV		(L1_S_TEX(0)|0|0)
#define L1_MODE_PTE		(L1_S_TEX(0)|0|L1_S_B)
#define L1_MODE_DISPLAY		(L1_S_TEX(0)|L1_S_C|0)
#define L1_MODE_MEMORY		(L1_S_TEX(0)|L1_S_C|L1_S_B)

#define L2_TABLE_SIZE	(1024)

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
 *      	INNER	OUTER
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
#define NMRR_DEFAULT		0x00cc00c0
#define PRRR_DEFAULT		0xf00a00a9

/* 
 * NON-TEX Access permission bits
 */
#define AP_PF		0x00		/* all accesses generate permission faults */
#define AP_KR		0x05		/* kernel read */
#define AP_KRW		0x01		/* kernel read/write */
#define AP_KRWUR	0x02		/* kernel read/write usr read */
#define AP_KRWURW	0x03		/* kernel read/write usr read/write */

/*
 * Domain Types for the Domain Access Control Register.
 */
#define	DOMAIN_FAULT	0x00		/* no access */
#define	DOMAIN_CLIENT	0x01		/* client */
#define	DOMAIN_RESERVED	0x02		/* reserved */
#define	DOMAIN_MANAGER	0x03		/* manager */

#define PMAP_DOMAIN_KERNEL	0	/* The kernel uses domain #0 */

/* physical page mask */
#define PTE_RPGN 0xfffff000

/// REWRITE

#define L2_L_SIZE       0x00010000      /* 64K */
#define L2_L_OFFSET     (L2_L_SIZE - 1)
#define L2_L_FRAME      (~L2_L_OFFSET)
#define L2_L_SHIFT      16

#define L2_S_SIZE       0x00001000      /* 4K */
#define L2_S_OFFSET     (L2_S_SIZE - 1)
#define L2_S_FRAME      (~L2_S_OFFSET)
#define L2_S_SHIFT      12

///

#endif /* _ARM_PTE_H_ */
