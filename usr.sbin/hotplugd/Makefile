# $OpenBSD: Makefile,v 1.15 2013/05/30 15:55:53 florian Exp $

.if ${MACHINE} == "amd64" || \
    ${MACHINE} == "i386" || \
    ${MACHINE} == "armv7" || \
    ${MACHINE} == "imx"

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
