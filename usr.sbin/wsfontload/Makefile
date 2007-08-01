#	$OpenBSD: Makefile,v 1.10 2007/08/01 21:32:55 deraadt Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "alpha" || ${MACHINE} == "hppa" || \
    ${MACHINE} == "zaurus"

PROG=	wsfontload
SRCS=	wsfontload.c

.else

NOPROG=yes

.endif

MAN=	wsfontload.8
MANSUBDIR=	alpha hppa i386 amd64 cats zaurus

.include <bsd.prog.mk>
