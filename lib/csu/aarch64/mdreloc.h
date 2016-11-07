/*	$OpenBSD: archdep.h,v 1.5 2014/01/19 10:25:45 guenther Exp $	*/

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

#ifndef _AARCH64_RELOC_H_
#define _AARCH64_RELOC_H_

#define	DL_MALLOC_ALIGN	8	/* Arch constraint or otherwise */

#define	MACHID	EM_AMD64	/* ELF e_machine ID value checked */

#define	RELTYPE	Elf64_Rela
#define	RELSIZE	sizeof(Elf64_Rela)

#include <sys/mman.h>
#include <elf_abi.h>
#include <machine/reloc.h>
#include "syscall.h"
#include "util.h"

static inline void
RELOC_REL(Elf64_Rel *r, const Elf64_Sym *s, Elf64_Addr *p, unsigned long v)
{
	/* AMD64 is a rela architecture */
}

static inline void
RELOC_RELA(Elf64_Rela *r, const Elf64_Sym *s, Elf64_Addr *p, unsigned long v,
    Elf_Addr *pltgot, long tlssize)
{
	if (ELF64_R_TYPE(r->r_info) == R_AARCH64_RELATIVE) {
		*p = v + r->r_addend;
	} else if (ELF64_R_TYPE(r->r_info) == R_AARCH64_GLOB_DAT) {
		*p = v + s->st_value + r->r_addend;
	} else if (ELF64_R_TYPE(r->r_info) == R_AARCH64_ABS64) {
		*p = v + s->st_value + r->r_addend;
	} else if (ELF64_R_TYPE(r->r_info) == R_AARCH64_TPOFF64) {
			/*
			*where = (Elf_Addr) this->st_value -
			(Elf_Addr) refobj->tls_offset +
			(Elf_Addr) rels->r_addend;
			*/
		*p = - tlssize + r->r_addend;
	} else {
#if !defined(RCRT0)
		_dl_printf("unknown bootstrap relocation\n");
#endif
		_dl_exit(6);
	}
}

#define RELOC_GOT(obj, offs)

#define GOT_PERMS PROT_READ

#endif /* _AARCH64_RELOC_H_ */
