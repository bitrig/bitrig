/*	$OpenBSD: ctype.h,v 1.24 2014/05/26 01:49:36 guenther Exp $	*/
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

int     isalnum(int);
int     isalpha(int);
int     iscntrl(int);
int     isdigit(int);
int     isgraph(int);
int     islower(int);
int     isprint(int);
int     ispunct(int);
int     isspace(int);
int     isupper(int);
int     isxdigit(int);
int     tolower(int);
int     toupper(int);

#if __BSD_VISIBLE || __XPG_VISIBLE
int     isascii(int);
int     toascii(int);
#endif

#if __ISO_C_VISIBLE >= 1999
int     isblank(int);
#endif

#if __BSD_VISIBLE
int     digittoint(int);
int     ishexnumber(int);
int     isideogram(int);
int     isnumber(int);
int     isphonogram(int);
int     isrune(int);
int     isspecial(int);
#endif

#if __POSIX_VISIBLE >= 200809 || defined(_XLOCALE_H_)
#include <xlocale/_ctype.h>
#endif

#ifndef __cplusplus
#define	isalnum(_c)	__sbistype((_c), _CTYPE_A|_CTYPE_D)
#define	isalpha(_c)	__sbistype((_c), _CTYPE_A)
#define	iscntrl(_c)	__sbistype((_c), _CTYPE_C)
#define	isdigit(_c)	__isctype((_c), _CTYPE_D) /* ANSI -- locale independent */
#define	isgraph(_c)	__sbistype((_c), _CTYPE_G)
#define	islower(_c)	__sbistype((_c), _CTYPE_L)
#define	isprint(_c)	__sbistype((_c), _CTYPE_R)
#define	ispunct(_c)	__sbistype((_c), _CTYPE_P)
#define	isspace(_c)	__sbistype((_c), _CTYPE_S)
#define	isupper(_c)	__sbistype((_c), _CTYPE_U)
#define	isxdigit(_c)	__isctype((_c), _CTYPE_X) /* ANSI -- locale independent */
#define	tolower(_c)	__sbtolower(_c)
#define	toupper(_c)	__sbtoupper(_c)
#endif /* !__cplusplus */

#endif /* !_ANSI_LIBRARY */

__END_DECLS

#endif /* !_CTYPE_H_ */
