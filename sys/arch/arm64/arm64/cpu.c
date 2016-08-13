/*	$OpenBSD: cpu.c,v 1.59 2015/12/25 08:34:50 visa Exp $ */

/*
 * Copyright (c) 2016 Dale Rahn <drahn@dalerahn.com>
 * Copyright (c) 1997-2004 Opsycon AB (www.opsycon.se)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <dev/rndvar.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/fdt.h>
#include <arm64/arm64/arm64var.h>

int	cpumatch(struct device *, void *, void *);
void	cpuattach(struct device *, struct device *, void *);

#ifdef MULTIPROCESSOR
extern struct cpu_info cpu_info_primary;
struct cpu_info *cpu_info_list = &cpu_info_primary;
struct cpuset cpus_running;
struct cpu_info *cpu_info_secondaries[MAXCPUS];
void cpu_boot_secondary(struct cpu_info *ci);
void cpu_hatch_secondary(struct cpu_info *ci);

#endif

void
cpuattach(struct device *parent, struct device *dev, void *aux)
{
	struct arm64_attach_args *ca = aux;
	struct cpu_info *ci;
	int cpuno = dev->dv_unit;

	if (cpuno == 0) {
		ci = &cpu_info_primary;
#ifdef MULTIPROCESSOR
		ci->ci_flags |= CPUF_RUNNING | CPUF_PRESENT | CPUF_PRIMARY;
		cpuset_add(&cpus_running, ci);
#endif
	}
#ifdef MULTIPROCESSOR
	else {
		ci = malloc(sizeof(*ci), M_DEVBUF, M_WAITOK|M_ZERO);
		cpu_info_secondaries[cpuno - 1] = ci;
		cpu_info[cpuno] = ci;
		ci->ci_next = cpu_info_list->ci_next;
		cpu_info_list->ci_next = ci;
		ci->ci_flags |= CPUF_AP;
	}
#else
	else {
		printf("cpu skipped\n");
	}
#endif

	ci->ci_cpuid = cpuno;
	ci->ci_dev = dev;

	printf(":");

	void *node = ca->aa_node;
	int len;
	if (ca->aa_node != NULL) {
		// pull out fdt info.
		char *data;
		len = fdt_node_property(node, "enable-method", &data);
		if (len > 4 && strcmp ((char *)data, "psci") == 0) {
			uint32_t reg;
			len = fdt_node_property_ints(node, "reg", &reg, 1);
			if (len == 1) {
				ci->ci_mpidr = reg;
				printf(" psci %x", reg);
			} else {
				ci->ci_flags = 0;
				printf(" disabled", reg);
			}
		}
	}

	if (ci->ci_flags & CPUF_AP) {
		ncpusfound++;
		cpu_hatch_secondary(ci);
		atomic_setbits_int(&ci->ci_flags, CPUF_IDENTIFY);
		while ((ci->ci_flags & CPUF_IDENTIFIED) == 0)
			; /// timeout
	}

	// CPU INFO

	printf("\n");

}

#ifdef MULTIPROCESSOR
struct cpu_info *
get_cpu_info(int cpuno)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	CPU_INFO_FOREACH(cii, ci) {
		if (ci->ci_cpuid == cpuno)
			return ci;
	}
	return NULL;
}

int hvc(uint64_t a0, uint64_t a1, uint64_t a2);
void cpu_hatch(void);

void
cpu_boot_secondary_processors(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	extern uint64_t pmap_avail_kvo;


	CPU_INFO_FOREACH(cii, ci) {
		if ((ci->ci_flags & CPUF_AP) == 0)
			continue;
		if (ci->ci_flags & CPUF_PRIMARY)
			continue;

		ci->ci_randseed = (arc4random() & 0x7fffffff) + 1;
		sched_init_cpu(ci);
		cpu_boot_secondary(ci);

	}

      //mips64_ipi_init();
}

volatile int cpu_ready;
volatile int cpu_running;
void dcache_wbinv_poc(vaddr_t, size_t);
void
cpu_hatch_secondary(struct cpu_info *ci)
{
	extern uint64_t pmap_avail_kvo;
	extern paddr_t cpu_hatch_ci;

	//printf(" spinning up cpu %d %p", ci->ci_mpidr, ci);
	uint64_t kstack = uvm_km_alloc (kernel_map, USPACE+1024);
	if (kstack == 0) {
		panic("no stack for cpu\n");
	}
	ci->ci_el1_stkend = kstack +USPACE-0x16;
	ci->ci_el2_stkend = kstack +USPACE+512-16;
	ci->ci_el3_stkend = kstack +USPACE+1024-16;

	cpu_ready = 0;
	cpu_running = 0;
	pmap_extract(pmap_kernel(), (vaddr_t)ci, &cpu_hatch_ci);
	ci->ci_ci = (uint64_t)ci;

	uint64_t ttbr1;
	asm ("mrs %x0, ttbr1_el1": "=r"(ttbr1));
	ci->ci_ttbr1 = ttbr1;
	dcache_wbinv_poc((vaddr_t)&cpu_hatch_ci, 8);
	dcache_wbinv_poc((vaddr_t)ci, sizeof(*ci));

	hvc(0xc4000003, ci->ci_mpidr, (uint64_t)cpu_hatch+pmap_avail_kvo);
}

void 
cpu_boot_secondary(struct cpu_info *ci)
{
	atomic_setbits_int(&ci->ci_flags, CPUF_GO);
	asm ("dsb sy");
	asm ("sev");

	while ((ci->ci_flags & CPUF_RUNNING) == 0) {
		delay(10);
	}
}

void
cpu_start_secondary(struct cpu_info *ci)
{
	int s;

	ncpus++;
	ci->ci_flags |= CPUF_PRESENT;
	asm ("dsb sy");

	while ((ci->ci_flags & CPUF_IDENTIFY) == 0) {
		delay (10);
	}
	printf(" cpu%d %p running", ci->ci_mpidr, ci);
	atomic_setbits_int(&ci->ci_flags, CPUF_IDENTIFIED);
	asm ("dsb sy");

	while ((ci->ci_flags & CPUF_GO) == 0) {
		asm ("wfe");
	}

	s=splhigh();
	cpu_startclock();

	nanouptime(&ci->ci_schedstate.spc_runtime);

	spllower(IPL_NONE);

	atomic_setbits_int(&ci->ci_flags, CPUF_RUNNING);

        SCHED_LOCK(s);
	cpu_switchto(NULL, sched_chooseproc());
}

void
cpu_unidle(struct cpu_info *ci)
{
	if (ci != curcpu())
		//mips64_send_ipi(ci->ci_cpuid, MIPS64_IPI_NOP);
		// should this be sev or ipi ?
		asm volatile ("sev");
}
#endif
