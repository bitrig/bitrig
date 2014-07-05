/*
 * Copyright (c) 2014 Martin Natano <natano@natano.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <assert.h>
#include <locale.h>
#include <err.h>

#define AUML	(0x00e4)

int
main(void)
{
	FILE *f;
	wint_t wc;
	int c, n;
	char utxt[] = {0xc3, 0xa4, 'f', 'o', 'o'};


	/*
	 * Set up test file.
	 */

	f = tmpfile();
	assert(f != NULL);
	n = fwrite(utxt, 1, sizeof(utxt), f);
	assert(n == sizeof(utxt));
	rewind(f);


	/*
	 * Actual tests.
	 */

	setlocale(LC_CTYPE, "en_US.UTF-8");

	printf("reading auml byte pattern - ");
	c = getc(f);
	assert(c == 0xc3);
	c = getc(f);
	assert(c == 0xa4);
	printf("ok\n");

	printf("ungetc()in auml byte pattern - ");
	assert(ungetc(0xa4, f) == 0xa4);
	assert(ungetc(0xc3, f) == 0xc3);
	printf("ok\n");

	printf("reading wide character - ");
	wc = getwc(f);
	assert(wc == AUML);
	printf("ok\n");

	printf("ungetwc()ing wide character - ");
	assert(ungetwc(wc, f) == wc);
	printf("ok\n");

	printf("reading auml byte pattern - ");
	c = getc(f);
	assert(c == 0xc3);
	c = getc(f);
	assert(c == 0xa4);
	printf("ok\n");

	printf("ungetc()in auml byte pattern - ");
	assert(ungetc(0xa4, f) == 0xa4);
	assert(ungetc(0xc3, f) == 0xc3);
	printf("ok\n");

	printf("reading 4 wide characters - ");
	wc = getwc(f);
	assert(wc == AUML);
	wc = getwc(f);
	assert(wc == L'f');
	wc = getwc(f);
	assert(wc == L'o');
	wc = getwc(f);
	assert(wc == L'o');
	printf("ok\n");

	printf("ungetwc()ing 4 wide characters - ");
	assert(ungetwc(L'o', f) == L'o');
	assert(ungetwc(L'o', f) == L'o');
	assert(ungetwc(L'f', f) == L'f');
	assert(ungetwc(AUML, f) == AUML);
	printf("ok\n");

	printf("reading 4 wide characters again - ");
	wc = getwc(f);
	assert(wc == AUML);
	wc = getwc(f);
	assert(wc == L'f');
	wc = getwc(f);
	assert(wc == L'o');
	wc = getwc(f);
	assert(wc == L'o');
	printf("ok\n");

	fclose(f);
	return (0);
}
