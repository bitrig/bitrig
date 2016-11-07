/*	$OpenBSD: stdarg.h,v 1.9 2011/03/23 16:54:34 pirofti Exp $ */
/*	$NetBSD: stdarg.h,v 1.7 2003/08/07 16:26:53 agc Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)stdarg.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _MACHINE_STDARG_H_
#define	_MACHINE_STDARG_H_

#include <sys/cdefs.h>
#include <machine/_types.h>

typedef __va_list	va_list;
#ifdef __lint__
#define __builtin_next_arg(t)		((t) ? 0 : 0)
#define	__builtin_stdarg_start(a, l)	((a) = ((l) ? 0 : 0))
#define	__builtin_va_arg(a, t)		((a) ? 0 : 0)
#define	__builtin_va_end		/* nothing */
#define	__builtin_va_copy(d, s)		((d) = (s))
#endif

#define	va_start(ap, last)	__builtin_stdarg_start((ap), (last))

#define	va_arg(ap, type)	__builtin_va_arg((ap), type)
#define	va_end(ap)		__builtin_va_end((ap))
#define	__va_copy(dest, src)	__builtin_va_copy((dest), (src))

#endif /* !_MACHINE_STDARG_H_ */
