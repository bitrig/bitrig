/*	$OpenBSD: isctype.c,v 1.11 2005/08/08 08:05:34 espie Exp $ */
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
 */

#define _ANSI_LIBRARY
#include <ctype.h>
#include <stdio.h>

int
isalnum(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L|_N)));
}
DEF_STRONG(isalnum);

int
isalpha(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L)));
}
DEF_STRONG(isalpha);

int
isblank(int c)
{
	return (c == ' ' || c == '\t');
}
DEF_STRONG(isblank);

int
iscntrl(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _C));
}
DEF_STRONG(iscntrl);

int
isdigit(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _N));
}
DEF_STRONG(isdigit);

int
isgraph(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N)));
}
DEF_STRONG(isgraph);

int
islower(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _L));
}
DEF_STRONG(islower);

int
isprint(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N|_B)));
}
DEF_STRONG(isprint);

int
ispunct(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _P));
}
DEF_STRONG(ispunct);

int
isspace(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _S));
}
DEF_STRONG(isspace);

int
isupper(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _U));
}
DEF_STRONG(isupper);

int
isxdigit(int c)
{
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_N|_X)));
}
DEF_STRONG(isxdigit);

int
isascii(int c)
{
	return ((unsigned int)c <= 0177);
}
DEF_WEAK(isascii);

int
toascii(int c)
{
	return (c & 0177);
}
DEF_STRONG(toascii);

int
_toupper(int c)
{
	return (c - 'a' + 'A');
}

int
_tolower(int c)
{
	return (c - 'A' + 'a');
}
