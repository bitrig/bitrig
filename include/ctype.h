/*	$OpenBSD: ctype.h,v 1.23 2014/03/16 18:38:30 guenther Exp $	*/
/*	$NetBSD: ctype.h,v 1.14 1994/10/26 00:55:47 cgd Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ctype.h	5.3 (Berkeley) 4/3/91
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

#include <sys/cdefs.h>
#include <_ctype.h>

#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_X	0x40
#define	_B	0x80

__BEGIN_DECLS

extern const char	*_ctype_;
extern const short	*_tolower_tab_;
extern const short	*_toupper_tab_;


#ifndef _ANSI_LIBRARY

#ifndef __cplusplus
#define	isalnum(c)	__sbistype((c), _CTYPE_A|_CTYPE_D)
#define	isalpha(c)	__sbistype((c), _CTYPE_A)
#define	iscntrl(c)	__sbistype((c), _CTYPE_C)
#define	isdigit(c)	__isctype((c), _CTYPE_D) /* ANSI -- locale independent */
#define	isgraph(c)	__sbistype((c), _CTYPE_G)
#define	islower(c)	__sbistype((c), _CTYPE_L)
#define	isprint(c)	__sbistype((c), _CTYPE_R)
#define	ispunct(c)	__sbistype((c), _CTYPE_P)
#define	isspace(c)	__sbistype((c), _CTYPE_S)
#define	isupper(c)	__sbistype((c), _CTYPE_U)
#define	isxdigit(c)	__isctype((c), _CTYPE_X) /* ANSI -- locale independent */
#define	tolower(c)	__sbtolower(c)
#define	toupper(c)	__sbtoupper(c)
#endif /* !__cplusplus */

#endif /* !_ANSI_LIBRARY */

__END_DECLS

#endif /* !_CTYPE_H_ */
