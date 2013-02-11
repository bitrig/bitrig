/* $FreeBSD$ */

#include "zz.h"

int __thread yy1 = 101;

int zz_init(int);


struct tls_be_scared __thread __tls_be_scared = {
	0x123,
	123.456,
	zz_init
};

int
zz_init(int a)
{
	return a + 1;
}
