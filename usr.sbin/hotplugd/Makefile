# $OpenBSD: Makefile,v 1.13 2010/07/20 02:05:51 deraadt Exp $

.if ${MACHINE} == "amd64" || \
    ${MACHINE} == "i386"

PROG=	hotplugd
SRCS=	hotplugd.c

CFLAGS+= -Wall
CFLAGS+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -Wmissing-declarations
CFLAGS+= -Wshadow -Wpointer-arith -Wcast-qual

.else

NOPROG=	yes

.endif

MAN=	hotplugd.8

.include <bsd.prog.mk>
