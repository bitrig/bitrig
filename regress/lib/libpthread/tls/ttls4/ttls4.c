/*
 * This program tests if a new thread's initial tls data
 * is clean.
 *
 * David Xu <davidxu@freebsd.org>
 *
 * $FreeBSD$
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <machine/sysarch.h>

int __thread n;
int __thread m;
int cnt;

void *f1(void *arg)
{
	int *parg = arg;
	if (n != 0) {
		printf("\nbug, n == %d m == %d &n %p call %d \n", n, m, &n,
		    *parg);
	//	exit(1);
	}
	printf("\rcnt %05d", cnt);
	fflush(stdout);
	cnt++;
	n = 1;
	return (0);
}

int
main(int argc, char **argv)
{
	pthread_t td;
	int i;
	int arg;

	char **p;

	n = 2;
	for (i = 0; i < 1000; ++i) {
		arg = 1;
		pthread_create(&td, NULL, f1, &arg);
		pthread_join(td, NULL);
	}
	sleep(2);
	for (i = 0; i < 1000; ++i) {
		arg = 2;
		pthread_create(&td, NULL, f1, &arg);
		pthread_join(td, NULL);
	}
	printf("\n");
	return (0);
}
