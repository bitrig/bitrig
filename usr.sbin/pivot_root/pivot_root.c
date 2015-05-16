/*	$NetBSD: pivot_root.c,v 1.38 2012/03/13 18:40:55 ast Exp $	*/

/*-
 * Copyright (c) 2012 Adrian Steinmann <ast at NetBSD org>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * To test syscall checks:
 * cc -O2 -DTEST_RAW_INPUT_TO_SYSCALL=1 -o pivot_root pivot_root.c
 */

#include <sys/cdefs.h>

#include <sys/param.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <sys/pivot_root.h>

/*
 * Moves the root file system of the current process to the directory root_old,
 * makes root_new as the new root file system of the current process, and sets
 * root/cwd of all processes which had them on the current root to root_new.
 *
 * Restrictions:
 * The root_new must be a mountpoint and not the current root; root_old is
 * the relative path under root_new and on the same file system as root_new.
 */

static void
usage(void)
{
	(void)fprintf(stderr, "usage: pivot_root root_new root_old\n");
}

int
main(int argc, char *argv[])
{
	char real_new[PATH_MAX], real_old[PATH_MAX];
	size_t len;

#if TEST_RAW_INPUT_TO_SYSCALL

	if (pivot_root(argv[1], argv[2]) != 0) {
		perror("pivot_root");
		exit(errno);
	}

#else /* TEST_RAW_INPUT_TO_SYSCALL */

	if (argc != 3) {
		usage();
		exit(EINVAL);
	}

	/* check if root_new exists and normalize it */
	if (realpath(argv[1], real_new) == NULL) {
		fprintf(stderr, "invalid root_new '%s'\n", real_new);
		exit(EINVAL);
	}

	/* check if root_old exists and normalize it */
	if (realpath(argv[2], real_old) == NULL) {
		fprintf(stderr, "invalid root_old '%s'\n", real_old);
		exit(EINVAL);
	}

	/* check if root_old appears to be under root_new */
	len = strlen(real_new);
	if ((strlen(real_old) <= len)
		|| (strstr(real_old, real_new) != real_old)
			|| (real_old[len] != '/')) {
		fprintf(stderr,
		"root_old '%s' doesn't appear to be under root_new '%s'\n",
		real_old, real_new);
		exit(EINVAL);
	}

	/* do syscall with normalized pathnames */
	if (pivot_root(real_new, real_old) != 0) {
		perror("pivot_root");
		exit(errno);
	}

#endif /* TEST_RAW_INPUT_TO_SYSCALL */

	/* send init a SIGHUP, reopening /etc/tty */
	signal(1, (void *) 1);
	if (errno != 0) {
		perror("signal");
		exit(errno);
	}

	exit(0);
}
