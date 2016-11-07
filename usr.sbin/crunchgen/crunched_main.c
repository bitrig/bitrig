/* $OpenBSD: crunched_main.c,v 1.5 2015/12/06 12:00:16 tobias Exp $	 */

/*
 * Copyright (c) 1994 University of Maryland
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of U.M. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  U.M. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * U.M. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL U.M.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: James da Silva, Systems Design and Analysis Group
 *			   Computer Science Department
 *			   University of Maryland at College Park
 */
/*
 * crunched_main.c - main program for crunched binaries, it branches to a
 *	particular subprogram based on the value of argv[0].  Also included
 *	is a little program invoked when the crunched binary is called via
 *	its EXECNAME.  This one prints out the list of compiled-in binaries,
 *	or calls one of them based on argv[1].   This allows the testing of
 *	the crunched binary without creating all the links.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct stub {
	char	*name;
	int	(*f)();
};

extern struct stub entry_points[];
int crunched_usage(void);

int
main(int argc, char *argv[], char **envp)
{
	extern char	*__progname;
	struct stub	*ep;

	if (__progname == NULL || *__progname == '\0')
		crunched_usage();

	for (ep = entry_points; ep->name != NULL; ep++)
		if (!strcmp(__progname, ep->name))
			break;

	if (ep->name)
		return ep->f(argc, argv, envp);
	fprintf(stderr, "%s: %s not compiled in\n", EXECNAME, __progname);
	crunched_usage();
}

int
crunched_main(int argc, char **argv, char **envp)
{
	struct stub	*ep;
	int		columns, len;

	if (argc <= 1)
		crunched_usage();

	return main(--argc, ++argv, envp);
}

int
crunched_usage(void)
{
	int		columns = 0, len;
	struct stub	*ep;

	fprintf(stderr,
	    "Usage: %s <prog> <args> ..., where <prog> is one of:\n",
	    EXECNAME);
	for (ep = entry_points; ep->name != NULL; ep++) {
		len = strlen(ep->name) + 1;
		if (columns + len < 80)
			columns += len;
		else {
			fprintf(stderr, "\n");
			columns = len;
		}
		fprintf(stderr, " %s", ep->name);
	}
	fprintf(stderr, "\n");
	exit(1);
}
