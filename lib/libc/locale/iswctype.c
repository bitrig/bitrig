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
#include <stdio.h>

#include <wctype.h>

#undef iswalnum
int
iswalnum(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_A|_CTYPE_D));
}
DEF_STRONG(iswalnum);

#undef iswalpha
int
iswalpha(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_A));
}
DEF_STRONG(iswalpha);

#undef iswascii
int
iswascii(wc)
	wint_t wc;
{
	return ((wc & ~0x7F) == 0);
}
DEF_STRONG(iswascii);

#undef iswblank
int
iswblank(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_B));
}
DEF_STRONG(iswblank);

#undef iswcntrl
int
iswcntrl(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_C));
}
DEF_STRONG(iswctrl);

#undef iswdigit
int
iswdigit(wc)
	wint_t wc;
{
	return (__isctype(wc, _CTYPE_D));
}
DEF_STRONG(iswdigit);

#undef iswgraph
int
iswgraph(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_G));
}
DEF_STRONG(iswgraph);

#undef iswhexnumber 
int
iswhexnumber(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_X));
}
DEF_STRONG(iswhexnumber);

#undef iswideogram
int
iswideogram(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_I));
}
DEF_STRONG(iswideogram);

#undef iswlower
int
iswlower(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_L));
}
DEF_STRONG(iswlower);

#undef iswnumber
int
iswnumber(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_D));
}
DEF_STRONG(iswnumber);

#undef iswphonogram	
int
iswphonogram(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_Q));
}
DEF_STRONG(iswphonogram);

#undef iswprint
int
iswprint(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_R));
}
DEF_STRONG(iswprint);

#undef iswpunct
int
iswpunct(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_P));
}
DEF_STRONG(iswpunct);

#undef iswrune
int
iswrune(wc)
	wint_t wc;
{
	return (__istype(wc, 0xFFFFFF00L));
}
DEF_STRONG(iswrune);

#undef iswspace
int
iswspace(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_S));
}
DEF_STRONG(iswspace);

#undef iswspecial
int
iswspecial(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_T));
}
DEF_STRONG(iswspecial);

#undef iswupper
int
iswupper(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_U));
}
DEF_STRONG(iswupper);

#undef iswxdigit
int
iswxdigit(wc)
	wint_t wc;
{
	return (__isctype(wc, _CTYPE_X));
}
DEF_STRONG(iswxdigit);

#undef towlower
wint_t
towlower(wc)
	wint_t wc;
{
        return (__tolower(wc));
}
DEF_STRONG(towlower);

#undef towupper
wint_t
towupper(wc)
	wint_t wc;
{
        return (__toupper(wc));
}
DEF_STRONG(towupper);
