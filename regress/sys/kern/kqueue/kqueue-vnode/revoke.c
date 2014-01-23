#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s <file>\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	if (argc != 2)
		usage();

	if (revoke(argv[1]) == -1)
		err(EXIT_FAILURE, "revoke");

	exit(0);
}
