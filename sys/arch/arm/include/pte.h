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

/* Short-decriptor translation table First level descriptor format */
#define L1_S_B		(0x00000004)	/* bufferable Section */
#define L1_S_C		(0x00000008)	/* cacheable Section */
#define L1_S_AP(ap)	((((ap) & 0x4) << 13) | (((ap) & 3) << 10))
#define L1_S_TEX(x)	(((x)&0x7)<<12)
#define L1_S_XN(x)	(((x)&0x1)<<4)
#define L1_S_G(x)	(((x)&0x1)<<17)
#define L1_S_RPGN	(0xfff00000)
#define L1_S_SHIFT	(20)
#define	L1_S_SIZE	0x00100000	/* 1M */
#define	L1_S_OFFSET	(L1_S_SIZE-1)	/* 1M */
#define L1_TABLE_SIZE	(16 * 1024)

/* 
 * NON-TEX Access permission bits
 */
#define AP_PF           0x00            /* all accesses generate permission faul
ts */
#define AP_KR           0x05            /* kernel read */
#define AP_KRW          0x01            /* kernel read/write */
#define AP_KRWUR        0x02            /* kernel read/write usr read */
#define AP_KRWURW       0x03            /* kernel read/write usr read/write */

/*
 * Domain Types for the Domain Access Control Register.
 */
#define	DOMAIN_FAULT	0x00		/* no access */
#define	DOMAIN_CLIENT	0x01		/* client */
#define	DOMAIN_RESERVED	0x02		/* reserved */
#define	DOMAIN_MANAGER	0x03		/* manager */

#define PMAP_DOMAIN_KERNEL      15      /* The kernel uses domain #15 */

/* physical page mask */
#define PTE_RPGN 0xfffff000

/* XXX */
#ifndef _LOCORE
struct pte {
        uint32_t pte;
};

typedef uint32_t pd_entry_t;	/* L1 table entry */
typedef uint32_t pt_entry_t;	/* L2 table entry */

struct pv_node {
};


/*
deleteme
*/

union pmap_cache_state {
	struct {
		union {
			u_int8_t csu_cache_b[2];
			u_int16_t csu_cache;
		} cs_cache_u;

		union {
			u_int8_t csu_tlb_b[2];
			u_int16_t csu_tlb;
		} cs_tlb_u;
	} cs_s;
	u_int32_t cs_all;
};
#define	cs_cache_id	cs_s.cs_cache_u.csu_cache_b[0]
#define	cs_cache_d	cs_s.cs_cache_u.csu_cache_b[1]
#define	cs_cache	cs_s.cs_cache_u.csu_cache
#define	cs_tlb_id	cs_s.cs_tlb_u.csu_tlb_b[0]
#define	cs_tlb_d	cs_s.cs_tlb_u.csu_tlb_b[1]
#define	cs_tlb		cs_s.cs_tlb_u.csu_tlb

#endif /* _LOCORE */


/// REWRITE
#define L2_L_SIZE       0x00010000      /* 64K */
#define L2_L_OFFSET     (L2_L_SIZE - 1)
#define L2_L_FRAME      (~L2_L_OFFSET)
#define L2_L_SHIFT      16

#define L2_S_SIZE       0x00001000      /* 4K */
#define L2_S_OFFSET     (L2_S_SIZE - 1)
#define L2_S_FRAME      (~L2_S_OFFSET)
#define L2_S_SHIFT      12

#define L2_TABLE_SIZE (4 *1024)

///

#endif /* _ARM_PTE_H_ */
