/*
 * Copyright (c) 2011 Dale Rahn <drahn@openbsd.org>
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
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <arm/include/vfp.h>
#include <arm/include/undefined.h>

void vfp_store(struct vfp_sp_state *vfpsave);

#define set_vfp_fpexc(val)						\
	__asm __volatile("mcr p10, 7, %0, c8, c0, 0" :: 		\
	    "r" (val))

#define get_vfp_fpexc()							\
({									\
	uint32_t val = 0;						\
	__asm __volatile("mrc p10, 7, %0, c8, c0, 0" :	 		\
	    "=r" (val));						\
	val;								\
})

int vfp_fault(unsigned int, unsigned int, trapframe_t *, int);
void vfp_load(struct proc *p);

void
vfp_init(void)
{
	uint32_t val;
	static int inited = 0;

	if (inited == 1)
		return;
	inited = 1;

	install_coproc_handler(10, vfp_fault);
	install_coproc_handler(11, vfp_fault);

	/* Read Coprocessor Access Control Register */
	__asm __volatile("mrc p15, 0, %0, c1, c0, 2" : "=r" (val) : : "cc");

	val |= COPROC10 | COPROC11;

	__asm __volatile("mcr p15, 0, %0, c1, c0, 2\n\t"
	    "isb\n\t"
	    : : "r" (val) : "cc");

	/* other stuff?? */
}

void
vfp_store(struct vfp_sp_state *vfpsave)
{
	uint32_t		 scratch;

	if (get_vfp_fpexc() & VFPEXC_EN) {
		__asm __volatile(
		    "stc	p11, c0, [%1], #128\n"		/* d0-d15 */
		    "stcl	p11, c0, [%1], #128\n"		/* d16-d31 */
		    "mrc	p10, 7, %0, c1, c0, 0\n"
		    "str	%0, [%1]\n"			/* save vfpscr */
		: "=&r" (scratch) : "r" (vfpsave));
	}

	/* disable FPU */
	set_vfp_fpexc(0);
}

void
vfp_save(void)
{
	struct cpu_info		*ci = curcpu();
	struct pcb 		*pcb;
	struct proc		*p;

	uint32_t cr_8;

	if (ci->ci_fpuproc == 0)
		return;

	cr_8 = get_vfp_fpexc();

	if ((cr_8 & VFPEXC_EN) == 0)
		return;	/* not enabled, nothing to do */

	if (cr_8 & VFPEXC_EX)
		panic("vfp exceptional data fault, time to write more code");
	p = curproc;
	pcb = curpcb;

	if (pcb->pcb_fpcpu == NULL || ci->ci_fpuproc == NULL ||
	    !(pcb->pcb_fpcpu == ci && ci->ci_fpuproc == p)) {
		/* disable fpu before panic, otherwise recurse */
		set_vfp_fpexc(0);

		panic("FPU unit enabled when curproc and curcpu dont agree %p %p %p", pcb->pcb_fpcpu, ci->ci_fpuproc == p, ci);
	}

	vfp_store(&p->p_addr->u_pcb.pcb_fpstate);

	/* NOTE: fpu state is saved but remains 'valid', as long as
	 * curpcb()->pcb_fpucpu == ci && ci->ci_fpuproc == curproc()
	 * is true FPU state is valid and can just be enabled without reload.
	 */
}

void
vfp_enable()
{
	struct cpu_info		*ci = curcpu();

	if (curproc->p_addr->u_pcb.pcb_fpcpu == ci &&
	    ci->ci_fpuproc == curproc) {
		disable_interrupts(I32_bit);

		/* FPU state is still valid, just enable and go */
		set_vfp_fpexc(VFPEXC_EN);
	}
}

void
vfp_load(struct proc *p)
{
	struct cpu_info		*ci = curcpu();
	struct pcb		*pcb = &p->p_addr->u_pcb;
	uint32_t		 scratch = 0;

	/* do not allow a partially synced state here */
	disable_interrupts(I32_bit);

	/*
	 * p->p_pcb->pcb_fpucpu _may_ not be NULL here, but the FPU state
	 * was synced on kernel entry, so we can steal the FPU state
	 * instead of signalling and waiting for it to save
	 */

	/* enable to be able to load ctx */
	set_vfp_fpexc(VFPEXC_EN);

	__asm __volatile(
	    "ldc	p11, c0, [%1], #128\n"		/* d0-d15 */
	    "ldcl	p11, c0, [%1], #128\n"		/* d16-d31 */
	    "ldr	%0, [%1]\n"			/* set old vfpscr */
	    "mcr	p10, 7, %0, c1, c0, 0\n"
	    : "=&r" (scratch) : "r" (&pcb->pcb_fpstate));

	ci->ci_fpuproc = p;
	pcb->pcb_fpcpu = ci;

	/* disable until return to userland */
	set_vfp_fpexc(0);

	enable_interrupts(I32_bit);
}


int
vfp_fault(unsigned int pc, unsigned int insn, trapframe_t *tf, int fault_code)
{
	struct proc		*p;
	struct pcb 		*pcb;
	struct cpu_info		*ci;

	p = curproc;
	pcb = &p->p_addr->u_pcb;
	ci = curcpu();

	if (get_vfp_fpexc() & VFPEXC_EN) {
		set_vfp_fpexc(0);
		panic("%s: we shall not fault when FPU is enabled",
		    __func__);
	}

	/* we should be able to ignore old state of pcb_fpcpu ci_fpuproc */
	if ((pcb->pcb_flags & PCB_FPU) == 0) {
		pcb->pcb_flags |= PCB_FPU;

		bzero (&pcb->pcb_fpstate, sizeof (pcb->pcb_fpstate));

		/* XXX - setround()? */
	}
	vfp_load(p);

	return 0;
}

void
vfp_discard()
{
	struct cpu_info		*ci = curcpu();
	if (curpcb->pcb_fpcpu == ci && ci->ci_fpuproc == curproc) {
		ci->ci_fpuproc = NULL;
		curpcb->pcb_fpcpu  = NULL;
	}
}
