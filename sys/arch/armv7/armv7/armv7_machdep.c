/*	$OpenBSD: armv7_machdep.c,v 1.21 2015/05/12 04:31:10 jsg Exp $ */
/*	$NetBSD: lubbock_machdep.c,v 1.2 2003/07/15 00:25:06 lukem Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Machine dependant functions for kernel setup for 
 * Intel DBPXA250 evaluation board (a.k.a. Lubbock).
 * Based on iq80310_machhdep.c
 */
/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Machine dependant functions for kernel setup for Intel IQ80310 evaluation
 * boards using RedBoot firmware.
 */

/*
 * DIP switches:
 *
 * S19: no-dot: set RB_KDB.  enter kgdb session.
 * S20: no-dot: set RB_SINGLE. don't go multi user mode.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/socket.h>

#include <machine/db_machdep.h>
#include <machine/bootconfig.h>
#include <machine/bus.h>

#include <arm/undefined.h>
#include <arm/machdep.h>
#include <arm/armv7/armv7var.h>
#include <armv7/armv7/armv7_machdep.h>
#include <machine/fdt.h>

#include <dev/cons.h>

#include <net/if.h>

#include <ddb/db_extern.h>

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x04000000)
#define KERNEL_VM_SIZE		VM_KERNEL_SPACE_SIZE

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = "";
u_int cpu_reset_address = 0;

vaddr_t physical_start;
vaddr_t physical_freestart;
vaddr_t physical_freeend;
vaddr_t physical_end;
u_int free_pages;
int physmem = 0;

/*int debug_flags;*/
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t armstack;
extern pv_addr_t kernelstack;

vaddr_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

uint32_t	board_id;

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	32
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL+KERNEL_PT_KERNEL_NUM)
				        /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	8	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

extern struct user *proc0paddr;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int   safepri = 0;

/* Prototypes */

char	bootargs[MAX_BOOT_STRING];
int	bootstrap_bs_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *);
void	process_kernel_args(char *);
void	parse_uboot_tags(void *);
void	consinit(void);
void	platform_bootconfig_dram(BootConfig *, psize_t *, psize_t *);

bs_protos(bs_notimpl);

#ifndef CONSPEED
#define CONSPEED B115200	/* What u-boot */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

/*
 * void boot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
__dead void
boot(int howto)
{
#ifdef DIAGNOSTIC
	/* info */
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);
#endif

	if (cold) {
		config_suspend_all(DVACT_POWERDOWN);
		if ((howto & (RB_HALT | RB_USERREQ)) != RB_USERREQ) {
			printf("The operating system has halted.\n");
			printf("Please press any key to reboot.\n\n");
			cngetc();
		}
		printf("rebooting...\n");
		delay(500000);
		platform_watchdog_reset();
		printf("reboot failed; spinning\n");
		while(1);
		/*NOTREACHED*/
	}

	/* Disable console buffering */
/*	cnpollc(1);*/

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if ((howto & RB_NOSYNC) == 0)
		bootsync(howto);

	if_downall();

	uvm_shutdown();
	splhigh();
	cold = 1;

	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	config_suspend_all(DVACT_POWERDOWN);

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if ((howto & RB_HALT) != 0) {
		if ((howto & RB_POWERDOWN) != 0) {
			printf("\nAttempting to power down...\n");
			delay(500000);
			platform_powerdown();
		}

		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	delay(500000);
	platform_watchdog_reset();
	printf("reboot failed; spinning\n");
	for (;;) ;
	/* NOTREACHED */
}

static __inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

/*
 * simple memory mapping function used in early bootstrap stage
 * before pmap is initialized.
 * ignores cacheability and does map the sections with nocache.
 */
static vaddr_t section_free = 0xfd000000; /* XXX - huh */

int
bootstrap_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	u_long startpa, pa, endpa;
	vaddr_t va;
	pd_entry_t *pagedir = read_ttb();
	/* This assumes PA==VA for page directory */

	va = section_free;

	startpa = bpa & ~L1_S_OFFSET;
	endpa = (bpa + size) & ~L1_S_OFFSET;
	if ((bpa + size) & L1_S_OFFSET)
		endpa += L1_S_SIZE;

	*bshp = (bus_space_handle_t)(va + (bpa - startpa));

	for (pa = startpa; pa < endpa; pa += L1_S_SIZE, va += L1_S_SIZE)
		pmap_map_section((vaddr_t)pagedir, va, pa,
		    PROT_READ | PROT_WRITE, PTE_NOCACHE);

	cpu_tlb_flushD();

	section_free = va;

	return 0;
}

/*
dead code aint much fun.
static void
copy_io_area_map(pd_entry_t *new_pd)
{
	pd_entry_t *cur_pd = read_ttb();
	vaddr_t va;

	for (va = 0xfd000000;
	     (cur_pd[va>>L1_S_SHIFT] & L1_TYPE_MASK) == L1_TYPE_S;
	     va += L1_S_SIZE) {

		new_pd[va>>L1_S_SHIFT] = cur_pd[va>>L1_S_SHIFT];
		if (va == (ARM_VECTORS_HIGH & ~(0x00400000 - 1)))
			break;
	}
}
*/

/*
 * u_int initarm(...)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg0, void *arg1, void *arg2)
{
	int i, physsegs;
	pv_addr_t fdt;
	paddr_t memstart;
	psize_t memsize;
	extern uint32_t esym; /* &_end if no symbols are loaded */
	extern uint32_t eramdisk; /* zero if no ramdisk is loaded */
	uint32_t kernel_end = eramdisk ? eramdisk : esym;
	uint32_t initrd = 0, initrdsize = 0; /* initrd information from fdt */

	board_id = (uint32_t)arg1;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	platform_disable_l2_if_needed();

	if (fdt_init(arg2)) {
		void *node;

		node = fdt_find_node("/memory");
		if (node == NULL || fdt_get_memory_address(node, 0, &mem))
			panic("initarm: no memory specificed");

		memstart = mem.addr;
		memsize = mem.size;
		physical_start = mem.addr;
		physical_end = MIN(mem.addr + mem.size, (paddr_t)-PAGE_SIZE);

		node = fdt_find_node("/chosen");
		if (node != NULL) {
			char *bootargs;
			if (fdt_node_property(node, "bootargs", &bootargs))
				process_kernel_args(bootargs);

			uint32_t einitrd = 0;
			fdt_node_property_int(node,
			    "linux,initrd-start", &initrd);
			fdt_node_property_int(node,
			    "linux,initrd-end", &einitrd);
			initrdsize = einitrd - initrd;
			if (initrd && einitrd && einitrd > initrd &&
			    initrd > (esym + initrdsize -
			    KERNEL_TEXT_BASE + physical_start)) {
				kernel_end = eramdisk =
				    round_page(esym) + round_page(initrdsize);
				bootstrap_bs_map(NULL, (bus_addr_t)initrd,
				    initrdsize, 0, (bus_space_handle_t *)&initrd);
			} else
				initrd = initrdsize = 0;
		}
	}

	/* XXX: Use FDT information. */
	platform_init();
	platform_disable_l2_if_needed();

	/* setup a serial console for very early boot */
	//consinit();

	/* Talk to the user */
	printf("\n%s booting ...\n", platform_boot_name());

	printf("arg0 %p arg1 %p arg2 %p\n", arg0, arg1, arg2);

	if (fdt_get_size(arg2) == 0) {
		parse_uboot_tags(arg2);

		/*
		 * Examine the boot args string for options we need to know about
		 * now.
		 */
		process_kernel_args(bootconfig.bootstring);

		/* normally u-boot will set up bootconfig.dramblocks */
		platform_bootconfig_dram(&bootconfig, &memstart, &memsize);

		/*
		 * Set up the variables that define the availablilty of
		 * physical memory.
		 *
		 * XXX pmap_bootstrap() needs an enema.
		 */
		physical_start = bootconfig.dram[0].address;
		physical_end = MIN((uint64_t)physical_start +
		    (bootconfig.dram[0].pages * PAGE_SIZE), (paddr_t)-PAGE_SIZE);
	}

	physical_freestart = (((unsigned long)kernel_end - KERNEL_TEXT_BASE +0xfff) & ~0xfff) + memstart;
	physical_freeend = MIN((uint64_t)memstart+memsize,
	    (paddr_t)-PAGE_SIZE);
	// reserve some memory which will get mapped as part of the kernel
	/* Define a macro to simplify memory allocation */
#define valloc_pages(var, np)                           \
	alloc_pages((var).pv_pa, (np));                 \
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)                            \
	(var) = physical_freestart;                     \
	physical_freestart += ((np) * PAGE_SIZE);       \
	if (physical_freeend < physical_freestart)      \
		panic("initarm: out of memory");        \
	free_pages -= (np);                             \
	memset((char *)(var), 0, ((np) * PAGE_SIZE));

	physmem = (physical_end - physical_start) / PAGE_SIZE;

	alloc_pages(systempage.pv_pa, 1);
	systempage.pv_va = vector_page;

	/* Allocate stacks for all modes */
	valloc_pages(armstack, ARM_STACK_PAGES);
	valloc_pages(kernelstack, UPAGES);

	/* Store irq and abt stack in curcpu. */
	curcpu()->ci_irqstack = armstack.pv_va + IRQ_STACK_TOP;
	curcpu()->ci_abtstack = armstack.pv_va + ABT_STACK_TOP;

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

	/*
	 * Allocate pages for an FDT copy.
	 */
	if (fdt_get_size(arg2) != 0) {
		uint32_t size = fdt_get_size(arg2);
		valloc_pages(fdt, round_page(size) / PAGE_SIZE);
		memcpy((void *)fdt.pv_pa, arg2, size);
	}

	pmap_bootstrap(KERNEL_VM_BASE, esym, physical_start, physical_end);
	uvm_setpagesize();        /* initialize PAGE_SIZE-dependent variables */

	printf("success thus far\n");
//	while(1)
//		;

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the FDT. */
/*
	if (fdt.pv_va && fdt.pv_pa)
		pmap_map_chunk(l1pagetable, fdt.pv_va, fdt.pv_pa,
		    round_page(fdt_get_size((void *)fdt.pv_pa)),
		    PROT_READ | PROT_WRITE, PTE_CACHE);
*/

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* be a client to all domains */
	cpu_domains(0x55555555);
	/* Switch tables */

	//cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	setttb(pmap_kernel()->l1_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	proc0paddr = (struct user *)kernelstack.pv_va;
	proc0.p_addr = proc0paddr;

	arm32_vector_init(vector_page, ARM_VEC_ALL);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */

	set_stackptr(PSR_IRQ32_MODE, curcpu()->ci_irqstack);
	set_stackptr(PSR_ABT32_MODE, curcpu()->ci_abtstack);
	set_stackptr(PSR_UND32_MODE,
	    kernelstack.pv_va + USPACE_UNDEF_STACK_TOP);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */

	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Now we can reinit the FDT, using the virtual address. */
	if (fdt.pv_va && fdt.pv_pa)
		fdt_init((void *)fdt.pv_va);

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	//uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	//    atop(physical_freestart), atop(physical_freeend), 0);

	// only first block was originally given to the UVM(?) provide the rest

	physsegs = MIN(bootconfig.dramblocks, VM_PHYSSEG_MAX);

	for (i = 1; i < physsegs; i++) {
		paddr_t dramstart = bootconfig.dram[i].address;
		paddr_t dramend = MIN((uint64_t)dramstart +
		    bootconfig.dram[i].pages * PAGE_SIZE, (paddr_t)-PAGE_SIZE);
		physmem += (dramend - dramstart) / PAGE_SIZE;
		uvm_page_physload(atop(dramstart), atop(dramend),
		    atop(dramstart), atop(dramend), 0);
	}

	consinit();

	/*
	 * Make sure ramdisk is mapped, if there is any.
	 */
	if (eramdisk) {
		uint32_t ramdisk = roundup(esym, PAGE_SIZE);
		uint32_t pramdisk = physical_start + (ramdisk - KERNEL_TEXT_BASE);
		while (ramdisk < eramdisk) {
			pmap_kenter_pa(ramdisk, pramdisk, PROT_READ|PROT_WRITE);
			ramdisk += PAGE_SIZE;
			pramdisk += PAGE_SIZE;
		}
		/*
		 * Copy u-boot-passed ramdisk now, if there's any.
		 */
		if (initrd && initrdsize)
			memcpy((char *)round_page(esym), (char *)initrd,
			    initrdsize);
	}

#ifdef DDB
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init();

	if (boothowto & RB_KDB)
		Debugger();
#endif
	platform_print_board_type();

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}


void
process_kernel_args(char *args)
{
	char *cp = args;

	if (cp == NULL) {
		boothowto = RB_AUTOBOOT;
		return;
	}

	boothowto = 0;

	/* Make a local copy of the bootargs */
	strncpy(bootargs, cp, MAX_BOOT_STRING - sizeof(int));

	cp = bootargs;
	boot_file = bootargs;

	/* Skip the kernel image filename */
	while (*cp != ' ' && *cp != 0)
		++cp;

	if (*cp != 0)
		*cp++ = 0;

	while (*cp == ' ')
		++cp;

	boot_args = cp;

	printf("bootfile: %s\n", boot_file);
	printf("bootargs: %s\n", boot_args);

	/* Setup pointer to boot flags */
	while (*cp != '-')
		if (*cp++ == '\0')
			return;

	for (;*++cp;) {
		int fl;

		fl = 0;
		switch(*cp) {
		case 'a':
			fl |= RB_ASKNAME;
			break;
		case 'c':
			fl |= RB_CONFIG;
			break;
		case 'd':
			fl |= RB_KDB;
			break;
		case 's':
			fl |= RB_SINGLE;
			break;
		case '1':
			comcnspeed = B115200;
			break;
		case '9':
			comcnspeed = B9600;
			break;
		default:
			printf("unknown option `%c'\n", *cp);
			break;
		}
		boothowto |= fl;
	}
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	platform_init_cons();
}

void
board_startup(void)
{
        if (boothowto & RB_CONFIG) {
#ifdef BOOT_CONFIG
		user_config();
#else
		printf("kernel does not support -c; continuing..\n");
#endif
	}
}

void
platform_bootconfig_dram(BootConfig *bootconfig, psize_t *memstart, psize_t *memsize)
{
	int loop;

	if (bootconfig->dramblocks == 0)
		panic("%s: dramblocks not set up!", __func__);

	*memstart = bootconfig->dram[0].address;
	*memsize = bootconfig->dram[0].pages * PAGE_SIZE;
	printf("memory size derived from u-boot\n");
	for (loop = 0; loop < bootconfig->dramblocks; loop++) {
		printf("bootconf.mem[%d].address = %08x pages %d/0x%08x\n",
		    loop, bootconfig->dram[loop].address, bootconfig->dram[loop].pages,
		        bootconfig->dram[loop].pages * PAGE_SIZE);
	}
}
