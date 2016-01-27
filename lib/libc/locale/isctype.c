/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>

#include <ctype.h>

// This file implements functions which should be inlined
// into all callers, however for ABI purposes, the
// functions must export their strong/weak themselves
// without using PROTO_NORMAL or DEF_(STRONG|WEAK) as the
// inlined/defined version are affected by those interfaces.

#undef digittoint
int digittoint(int c) __attribute((weak));
int
digittoint(c)
	int c;
{
	return (__sbmaskrune(c, 0xFF));
}
//DEF_WEAK(digittoint);

#undef isalnum
int
isalnum(c)
	int c;
{
	return (__sbistype(c, _CTYPE_A|_CTYPE_D));
}
//DEF_STRONG(isalnum);

#undef isalpha
int isalpha(int c) __attribute((weak));
int
isalpha(c)
	int c;
{
	return (__sbistype(c, _CTYPE_A));
}
//DEF_WEAK(isalpha);

#undef isascii
int isascii(int c) __attribute((weak));
int
isascii(c)
	int c;
{
	return ((c & ~0x7F) == 0);
}
//DEF_WEAK(isascii);

#undef isblank
int
isblank(c)
	int c;
{
	return (__sbistype(c, _CTYPE_B));
}
//DEF_STRONG(isblank);

#undef iscntrl
int
iscntrl(c)
	int c;
{
	return (__sbistype(c, _CTYPE_C));
}
//DEF_STRONG(iscntrl);

#undef isdigit
int
isdigit(c)
	int c;
{
	return (__isctype(c, _CTYPE_D));
}
//DEF_STRONG(isdigit);

#undef isgraph
int
isgraph(c)
	int c;
{
	return (__sbistype(c, _CTYPE_G));
}
//DEF_STRONG(isgraph);

#undef ishexnumber 
int ishexnumber(int c) __attribute((weak));
int
ishexnumber(c)
	int c;
{
	return (__sbistype(c, _CTYPE_X));
}
//DEF_WEAK(ishexnumber);

#undef isideogram
int isideogram(int c) __attribute((weak));
int
isideogram(c)
	int c;
{
	return (__sbistype(c, _CTYPE_I));
}
//DEF_WEAK(isideogram);

#undef islower
int
islower(c)
	int c;
{
	return (__sbistype(c, _CTYPE_L));
}
//DEF_STRONG(islower);

#undef isnumber
int isnumber(int c) __attribute((weak));
int
isnumber(c)
	int c;
{
	return (__sbistype(c, _CTYPE_D));
}
//DEF_WEAK(isnumber);

#undef isphonogram	
int isphonogram(int c) __attribute((weak));
int
isphonogram(c)
	int c;
{
	return (__sbistype(c, _CTYPE_Q));
}
//DEF_WEAK(isphonogram);

#undef isprint
int
isprint(c)
	int c;
{
	return (__sbistype(c, _CTYPE_R));
}
//DEF_STRONG(isprint);

#undef ispunct
int
ispunct(c)
	int c;
{
	return (__sbistype(c, _CTYPE_P));
}
//DEF_STRONG(ispunct);

#undef isrune
int isrune(int c) __attribute((weak));
int
isrune(c)
	int c;
{
	return (__sbistype(c, 0xFFFFFF00L));
}
//DEF_WEAK(isrune);

#undef isspace
int
isspace(c)
	int c;
{
	return (__sbistype(c, _CTYPE_S));
}
//DEF_STRONG(isspace);

#undef isspecial
int isspecial(int c) __attribute((weak));
int
isspecial(c)
	int c;
{
	return (__sbistype(c, _CTYPE_T));
}
//DEF_WEAK(isspecial);

#undef isupper
int
isupper(c)
	int c;
{
	return (__sbistype(c, _CTYPE_U));
}
//DEF_STRONG(isupper);

#undef isxdigit
int
isxdigit(c)
	int c;
{
	return (__isctype(c, _CTYPE_X));
}
//DEF_STRONG(isxdigit);

#undef toascii
int toascii(int c) __attribute((weak));
int
toascii(c)
	int c;
{
	return (c & 0x7F);
}
//DEF_WEAK(toascii);

#undef tolower
int
tolower(c)
	int c;
{
	return (__sbtolower(c));
}
//DEF_STRONG(tolower);

#undef toupper
int
toupper(c)
	int c;
{
	return (__sbtoupper(c));
}
//DEF_STRONG(toupper);
