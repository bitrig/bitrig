/*	$OpenBSD: cpufunc.c,v 1.27 2015/05/29 05:48:07 jsg Exp $	*/
/*	$NetBSD: cpufunc.c,v 1.65 2003/11/05 12:53:15 scw Exp $	*/

/*
 * arm7tdmi support code Copyright (c) 2001 John Fremlin
 * arm8 support code Copyright (c) 1997 ARM Limited
 * arm8 support code Copyright (c) 1997 Causality Limited
 * arm9 support code Copyright (C) 2001 ARM Ltd
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufuncs.c
 *
 * C functions for supporting CPU / MMU / TLB specific operations.
 *
 * Created      : 30/01/97
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/pmap.h>
#include <arm/cpuconf.h>
#include <arm/cpufunc.h>

#if defined(PERFCTRS)
struct arm_pmc_funcs *arm_pmc;
#endif

/* PRIMARY CACHE VARIABLES */
int	arm_dcache_min_line_size = 32;
int	arm_icache_min_line_size = 32;
int	arm_idcache_min_line_size = 32;

int	arm_dcache_align;
int	arm_dcache_align_mask;

u_int	arm_cache_level;
u_int	arm_cache_type[14];
u_int	arm_cache_loc;

/* 1 == use cpu_sleep(), 0 == don't */
int cpu_do_powersave;

#ifdef CPU_ARMv7
struct cpu_functions armv7_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	armv7_setttb,			/* Setttb		*/
	cpufunc_dfsr,			/* dfsr			*/
	cpufunc_dfar,			/* dfar			*/
	cpufunc_ifsr,			/* ifsr			*/
	cpufunc_ifar,			/* ifar			*/

	/* TLB functions */

	armv7_tlb_flushID,		/* tlb_flushID		*/
	armv7_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv7_tlb_flushID,		/* tlb_flushI		*/
	armv7_tlb_flushID_SE,		/* tlb_flushI_SE	*/
	armv7_tlb_flushID,		/* tlb_flushD		*/
	armv7_tlb_flushID_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	armv7_icache_sync_all,		/* icache_sync_all	*/
	armv7_icache_sync_range,	/* icache_sync_range	*/

	armv7_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	armv7_dcache_wbinv_range,	/* dcache_wbinv_range	*/
	armv7_dcache_inv_range,		/* dcache_inv_range	*/
	armv7_dcache_wb_range,		/* dcache_wb_range	*/

	armv7_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv7_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	cpufunc_nullop,			/* sdcache_wbinv_all	*/
	(void *)cpufunc_nullop,		/* sdcache_wbinv_range	*/
	(void *)cpufunc_nullop,		/* sdcache_inv_range	*/
	(void *)cpufunc_nullop,		/* sdcache_wb_range	*/
	(void *)cpufunc_nullop,		/* sdcache_drain_writebuf	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv7_drain_writebuf,		/* drain_writebuf	*/

	armv7_cpu_sleep,		/* sleep (wait for interrupt) */

	/* Soft functions */
	armv7_context_switch,		/* context_switch	*/
	armv7_setup			/* cpu setup		*/
};

struct cpu_functions pj4bv7_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	armv7_drain_writebuf,		/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	armv7_setttb,			/* Setttb		*/
	cpufunc_dfsr,			/* dfsr			*/
	cpufunc_dfar,			/* dfar			*/
	cpufunc_ifsr,			/* ifsr			*/
	cpufunc_ifar,			/* ifar			*/

	/* TLB functions */

	armv7_tlb_flushID,		/* tlb_flushID		*/
	armv7_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv7_tlb_flushID,		/* tlb_flushI		*/
	armv7_tlb_flushID_SE,		/* tlb_flushI_SE	*/
	armv7_tlb_flushID,		/* tlb_flushD		*/
	armv7_tlb_flushID_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	armv7_icache_sync_all,		/* icache_sync_all	*/
	armv7_icache_sync_range,	/* icache_sync_range	*/

	armv7_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	armv7_dcache_wbinv_range,	/* dcache_wbinv_range	*/
	armv7_dcache_inv_range,		/* dcache_inv_range	*/
	armv7_dcache_wb_range,		/* dcache_wb_range	*/

	armv7_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv7_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	cpufunc_nullop,			/* sdcache_wbinv_all	*/
	(void *)cpufunc_nullop,		/* sdcache_wbinv_range	*/
	(void *)cpufunc_nullop,		/* sdcache_inv_range	*/
	(void *)cpufunc_nullop,		/* sdcache_wb_range	*/
	(void *)cpufunc_nullop,		/* sdcache_drain_writebuf	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv7_drain_writebuf,		/* drain_writebuf	*/

	pj4b_cpu_sleep,			/* sleep (wait for interrupt) */

	/* Soft functions */
	armv7_context_switch,		/* context_switch	*/
	pj4bv7_setup			/* cpu setup		*/
};
#endif /* CPU_ARMv7 */

/*
 * Global constants also used by locore.s
 */

struct cpu_functions cpufuncs;
u_int cputype;

#ifdef CPU_ARMv7
void
arm_get_cachetype_cp15v7(void)
{
	uint32_t csize, cachetype, clevel;
	uint32_t i, sel, type;

	/* Cache Type Register */
	__asm volatile("mrc p15, 0, %0, c0, c0, 1"
	    : "=r" (cachetype));

	arm_dcache_min_line_size = 1 << (CPU_CT_DMINLINE(cachetype) + 2);
	arm_icache_min_line_size = 1 << (CPU_CT_IMINLINE(cachetype) + 2);
	arm_idcache_min_line_size =
	    min(arm_icache_min_line_size, arm_dcache_min_line_size);

	/* CLIDR - Cache Level ID Register */
	__asm volatile("mrc p15, 1, %0, c0, c0, 1"
	    : "=r" (clevel));
	arm_cache_level = clevel;
	arm_cache_loc = CPU_CLIDR_LOC(arm_cache_level);

	i = 0;
	while ((type = clevel & 0x7) && i < 7) {
		if (type == CACHE_DCACHE || type == CACHE_UNI_CACHE ||
		    type == CACHE_SEP_CACHE) {
			sel = i << 1;
			/* CSSELR - Cache Size Selection Register */
			__asm volatile("mcr p15, 2, %0, c0, c0, 0"
			    : : "r" (sel));
			/* CCSIDR - Cache Size Identification Register */
			__asm volatile("mrc p15, 1, %0, c0, c0, 0"
			    : "=r" (csize));
			arm_cache_type[sel] = csize;
			arm_dcache_align = 1 <<
			    (CPUV7_CT_xSIZE_LEN(csize) + 4);
			arm_dcache_align_mask = arm_dcache_align - 1;
		}
		if (type == CACHE_ICACHE || type == CACHE_SEP_CACHE) {
			sel = (i << 1) | 1;
			/* CSSELR - Cache Size Selection Register */
			__asm volatile("mcr p15, 2, %0, c0, c0, 0"
			    : : "r" (sel));
			/* CCSIDR - Cache Size Identification Register */
			__asm volatile("mrc p15, 1, %0, c0, c0, 0"
			    : "=r" (csize));
			arm_cache_type[sel] = csize;
		}
		i++;
		clevel >>= 3;
	}
}
#endif /* CPU_ARMv7 */


/*
 * Cannot panic here as we may not have a console yet ...
 */

int
set_cpufuncs()
{
	cputype = cpufunc_id();
	cputype &= CPU_ID_CPU_MASK;

	/*
	 * NOTE: cpu_do_powersave defaults to off.  If we encounter a
	 * CPU type where we want to use it by default, then we set it.
	 */

#ifdef CPU_ARMv7
	if ((cputype & CPU_ID_CORTEX_A5_MASK) == CPU_ID_CORTEX_A5 ||
	    (cputype & CPU_ID_CORTEX_A7_MASK) == CPU_ID_CORTEX_A7 ||
	    (cputype & CPU_ID_CORTEX_A8_MASK) == CPU_ID_CORTEX_A8 ||
	    (cputype & CPU_ID_CORTEX_A9_MASK) == CPU_ID_CORTEX_A9 ||
	    (cputype & CPU_ID_CORTEX_A15_MASK) == CPU_ID_CORTEX_A15 ||
	    (cputype & CPU_ID_CORTEX_A17_MASK) == CPU_ID_CORTEX_A17) {
		cpufuncs = armv7_cpufuncs;
		arm_get_cachetype_cp15v7();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;
		return 0;
	}
	if ((cputype == CPU_ID_MV88SV581X_V6 ||
	    cputype == CPU_ID_MV88SV581X_V7 ||
	    cputype == CPU_ID_MV88SV584X_V7 ||
	    cputype == CPU_ID_ARM_88SV581X_V6 ||
	    cputype == CPU_ID_ARM_88SV581X_V7)) {
		cpufuncs = pj4bv7_cpufuncs;
		arm_get_cachetype_cp15v7();
		pmap_pte_init_armv7();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;
		return 0;
	}
#endif /* CPU_ARMv7 */
	/*
	 * Bzzzz. And the answer was ...
	 */
	panic("No support for this CPU type (%08x) in kernel", cputype);
	return(ARCHITECTURE_NOT_PRESENT);
}

/*
 * CPU Setup code
 */

#ifdef CPU_ARMv7
void
armv7_setup()
{
	uint32_t auxctl, oauxctl;
	int cpuctrl, cpuctrlmask;

	/* FIXME: Enable alignment faults after clang issue has been found. */
	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_BPRD_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_BPRD_ENABLE
	    | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
	    | CPU_CONTROL_ROUNDROBIN | CPU_CONTROL_CPCLK
	    | CPU_CONTROL_VECRELOC | CPU_CONTROL_FI | CPU_CONTROL_VE
	    | CPU_CONTROL_TRE | CPU_CONTROL_AFE;

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Now really make sure they are clean.  */
	/* XXX */
	/*
	asm volatile ("mcr\tp15, 0, r0, c7, c7, 0" : : );
	*/

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(cpuctrlmask, cpuctrl);

	if ((cputype & CPU_ID_CORTEX_A9_MASK) == CPU_ID_CORTEX_A9) {
		__asm __volatile("mrc p15, 0, %0, c1, c0, 1"
			: "=r" (auxctl));
		oauxctl = auxctl;

#ifdef MULTIPROCESSOR
		auxctl |= CORTEX_A9_AUXCTL_FW; /* Cache and TLB maintenance broadcast */
#endif
		extern int isomap;
		if (!isomap) {
			auxctl |= CORTEX_A9_AUXCTL_L1_PREFETCH_ENABLE;
			auxctl |= CORTEX_A9_AUXCTL_L2_PREFETCH_ENABLE;
		}
		auxctl |= CORTEX_A9_AUXCTL_SMP; /* needed for ldrex/strex */

		if (auxctl != oauxctl) {
			__asm __volatile("mcr p15, 0, %0, c1, c0, 1"
				: : "r" (auxctl));
		}
	}

	/* And again. */
	cpu_idcache_wbinv_all();
}

void
pj4bv7_setup()
{
	int cpuctrl;

	pj4b_config();

	cpuctrl = CPU_CONTROL_MMU_ENABLE;
	//cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
	cpuctrl |= CPU_CONTROL_DC_ENABLE;
	cpuctrl |= CPU_CONTROL_IC_ENABLE;
	cpuctrl |= (0xf << 3);
	cpuctrl |= CPU_CONTROL_BPRD_ENABLE;
	cpuctrl |= (0x5 << 16) | (1 < 22);
	cpuctrl |= CPU_CONTROL_XP;

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	cpu_control(0xffffffff, cpuctrl);

	/* And again. */
	cpu_idcache_wbinv_all();

	curcpu()->ci_ctrl = cpuctrl;
}
#endif	/* CPU_ARMv7 */
