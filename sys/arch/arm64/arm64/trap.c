/*-
 * Copyright (c) 2014 Andrew Turner
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>

#ifdef KDB
#include <sys/kdb.h>
#endif

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <machine/vfp.h>

#ifdef KDB
#include <machine/db_machdep.h>
#endif

#ifdef DDB
#include <ddb/db_output.h>
#endif

extern register_t fsu_intr_fault;

/* Called from exception.S */
void do_el1h_sync(struct trapframe *);
void do_el0_sync(struct trapframe *);
void do_el0_error(struct trapframe *);


#if 0
int
cpu_fetch_syscall_args(struct thread *td, struct syscall_args *sa)
{
	struct proc *p;
	register_t *ap;
	int nap;

	nap = 8;
	p = td->td_proc;
	ap = td->td_frame->tf_x;

	sa->code = td->td_frame->tf_x[8];

	if (sa->code == SYS_syscall || sa->code == SYS___syscall) {
		sa->code = *ap++;
		nap--;
	}

	if (p->p_sysent->sv_mask)
		sa->code &= p->p_sysent->sv_mask;
	if (sa->code >= p->p_sysent->sv_size)
		sa->callp = &p->p_sysent->sv_table[0];
	else
		sa->callp = &p->p_sysent->sv_table[sa->code];

	sa->narg = sa->callp->sy_narg;
	memcpy(sa->args, ap, nap * sizeof(register_t));
	if (sa->narg > nap)
		panic("TODO: Could we have more then 8 args?");

	td->td_retval[0] = 0;
	td->td_retval[1] = 0;

	return (0);
}

#include "../../kern/subr_syscall.c"

static void
svc_handler(struct trapframe *frame)
{
	struct syscall_args sa;
	struct thread *td;
	int error;

	td = curthread;
	td->td_frame = frame;

	error = syscallenter(td, &sa);
	syscallret(td, error, &sa);
}
#endif

void dumpregs(struct trapframe*);

static void
data_abort(struct trapframe *frame, uint64_t esr, int lower, int exe)
{
	struct vm_map *map;
	struct proc *p;
	struct pcb *pcb;
	vm_fault_t ftype;
	vm_prot_t access_type;
	vaddr_t va;
	union sigval sv;
	uint64_t far;
	int error = 0, sig;

	pcb = curcpu()->ci_curpcb;
	p = curcpu()->ci_curproc;

	/*
	 * Special case for fuswintr and suswintr. These can't sleep so
	 * handle them early on in the trap handler.
	 */
	if (__predict_false(pcb->pcb_onfault == (char *)&fsu_intr_fault)) {
		frame->tf_elr = (register_t)pcb->pcb_onfault;
		return;
	}

	far = READ_SPECIALREG(far_el1);

	if (lower)
		map = &p->p_vmspace->vm_map;
	else {
		/* The top bit tells us which range to use */
		if ((far >> 63) == 1)
			map = kernel_map;
		else
			map = &p->p_vmspace->vm_map;
	}

	va = trunc_page(far);
	if (exe)
		access_type = PROT_READ;
	else 
		access_type = ((esr >> 6) & 1) ? PROT_WRITE : PROT_READ;

	ftype = VM_FAULT_INVALID; // should check for failed permissions.

	if (map != kernel_map) {
		/*
		 * Keep swapout from messing with us during this
		 *	critical time.
		 */
		// XXX SMP
		//PROC_LOCK(p);
		//++p->p_lock;
		//PROC_UNLOCK(p);

		/* Fault in the user page: */
		if (!pmap_fault_fixup(map->pmap, va, access_type, 1)) {
			error = uvm_fault(map, va, ftype, access_type);
		}

		//PROC_LOCK(p);
		//--p->p_lock;
		//PROC_UNLOCK(p);
	} else {
		/*
		 * Don't have to worry about process locking or stacks in the
		 * kernel.
		 */
		if (!pmap_fault_fixup(map->pmap, va, access_type, 0)) {
			error = uvm_fault(map, va, ftype, access_type);
		}
	}

	if (error != 0) {
		if (lower) {
			if (error == ENOMEM)
				sig = SIGKILL;
			else
				sig = SIGSEGV;
			sv.sival_ptr = (u_int64_t *)far;
			dumpregs(frame);

			trapsignal(p, sig, 0, SEGV_ACCERR, sv);
		} else {
			if (curcpu()->ci_idepth == 0 &&
			    pcb->pcb_onfault != 0) {
				frame->tf_x[0] = error;
				frame->tf_elr = (register_t)pcb->pcb_onfault;
				return;
			}
			panic("uvm_fault failed: %lx", frame->tf_elr);
		}
	}

	if (lower)
		userret(p);
}

void
do_el1h_sync(struct trapframe *frame)
{
	uint32_t exception;
	uint64_t esr;

	/* Read the esr register to get the exception details */
	esr = READ_SPECIALREG(esr_el1);
	exception = ESR_ELx_EXCEPTION(esr);

	/*
	 * Sanity check we are in an exception er can handle. The IL bit
	 * is used to indicate the instruction length, except in a few
	 * exceptions described in the ARMv8 ARM.
	 *
	 * It is unclear in some cases if the bit is implementation defined.
	 * The Foundation Model and QEMU disagree on if the IL bit should
	 * be set when we are in a data fault from the same EL and the ISV
	 * bit (bit 24) is also set.
	 */
//	KASSERT((esr & ESR_ELx_IL) == ESR_ELx_IL ||
//	    (exception == EXCP_DATA_ABORT && ((esr & ISS_DATA_ISV) == 0)),
//	    ("Invalid instruction length in exception"));

	switch(exception) {
	case EXCP_FP_SIMD:
	case EXCP_TRAP_FP:
		panic("VFP exception in the kernel");
	case EXCP_DATA_ABORT:
		data_abort(frame, esr, 0, 0);
		break;
	case EXCP_BRK:
	case EXCP_WATCHPT_EL1:
	case EXCP_SOFTSTP_EL1:
#ifdef DDB
		{
		/* XXX */
		int db_trapper (u_int, u_int, trapframe_t *, int);
		db_trapper(frame->tf_elr, 0/*XXX*/, frame, exception);
		}
#else
		panic("No debugger in kernel.\n");
#endif
		break;
	default:
#ifdef DDB
		{
		/* XXX */
		int db_trapper (u_int, u_int, trapframe_t *, int);
		db_trapper(frame->tf_elr, 0/*XXX*/, frame, exception);
		}
#endif
		panic("Unknown kernel exception %x esr_el1 %lx lr %lxpc %lx\n",
		    exception,
		    esr, frame->tf_lr, frame->tf_elr);
	}
}

void
do_el0_sync(struct trapframe *frame)
{
	uint32_t exception;
	uint64_t esr;

	/* Check we have a sane environment when entering from userland */
//	KASSERT((uintptr_t)get_pcpu() >= VM_MIN_KERNEL_ADDRESS,
//	    ("Invalid pcpu address from userland: %p (tpidr %lx)",
//	     get_pcpu(), READ_SPECIALREG(tpidr_el1)));

	esr = READ_SPECIALREG(esr_el1);
	exception = ESR_ELx_EXCEPTION(esr);

	switch(exception) {
	case EXCP_FP_SIMD:
	case EXCP_TRAP_FP:
		vfp_fault(frame->tf_elr, 0, frame, exception);
		break;
	case EXCP_SVC:
		vfp_save();
		svc_handler(frame);
		break;
	case EXCP_INSN_ABORT_L:
		vfp_save();
		data_abort(frame, esr, 1, 1);
		break;
	case EXCP_DATA_ABORT_L:
		vfp_save();
		data_abort(frame, esr, 1, 0);
		break;
	default:
		// panic("Unknown userland exception %x esr_el1 %lx\n", exception,
		//    esr);
		// USERLAND MUST NOT PANIC MACHINE 
		{
			// only here to debug !?!?
			void dumpregs(struct trapframe *frame);
			dumpregs(frame);
		}
		sigexit(curproc, SIGILL);
		userret(curproc);
	}
}

void
do_el0_error(struct trapframe *frame)
{

	panic("do_el0_error");
}

