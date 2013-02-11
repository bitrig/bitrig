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
#include "zz.h"


void *f1(void *arg)
{
	int a;
	struct tls_be_scared *ptls = &__tls_be_scared;

	a = ptls->func(4);
	if (a != 4 + 1) {
		printf("failed a %d\n", a);
	}
	return NULL;
}

int
main(int argc, char **argv)
{
	pthread_t td;
	int i;
	int arg;

	char **p;
	struct tls_be_scared *ptls = &__tls_be_scared;

	ptls->func(4);

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
	return (0);
}
