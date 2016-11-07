/*	$OpenBSD: archdep.h,v 1.5 2014/07/05 12:22:41 miod Exp $ */

/*
 * Copyright (c) 1998 Per Fogelstrom, Opsycon AB
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

#ifndef _ARM_ARCHDEP_H_
#define _ARM_ARCHDEP_H_

#define	DL_MALLOC_ALIGN	4	/* Arch constraint or otherwise */

#define	MACHID	EM_AARCH64	/* ELF e_machine ID value checked */

#define	RELTYPE	Elf64_Rela
#define	RELSIZE	sizeof(Elf64_Rela)

#include <elf_abi.h>
#include <machine/reloc.h>
#include "syscall.h"
#include "util.h"

/*
 *	The following functions are declared inline so they can
 *	be used before bootstrap linking has been finished.
 */

static inline void *
_dl_mmap(void *addr, unsigned int len, unsigned int prot,
	unsigned int flags, int fd, off_t offset)
{
	return((void *)_dl__syscall((quad_t)SYS_mmap, addr, len, prot,
		flags, fd, 0, offset));
}

static inline void
RELOC_REL(Elf_Rel *r, const Elf_Sym *s, Elf_Addr *p, unsigned long v)
{
	_dl_exit(18);
}

static inline void
RELOC_RELA(Elf64_Rela *r, const Elf64_Sym *s, Elf64_Addr *p, unsigned long v,
    Elf_Addr *pltgot)
{
	if (ELF64_R_TYPE(r->r_info) == R_AARCH64_RELATIVE) {
		*p = v + r->r_addend;
	} else if (ELF64_R_TYPE(r->r_info) == R_AARCH64_GLOB_DAT) {
		*p = v + s->st_value + r->r_addend;
	} else {
		_dl_printf("unknown bootstrap relocation\n");
		_dl_exit(6);
	}
}

#define RELOC_GOT(obj, offs)

#define GOT_PERMS (PROT_READ|PROT_EXEC)

#endif /* _ARM_ARCHDEP_H_ */
