#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s <file> <length>\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	if (argc != 3)
		usage();

	if (truncate(argv[1], atoi(argv[2])) < 0)
		err(EXIT_FAILURE, "ftruncate");

	exit(0);
}
