#	$OpenBSD: Makefile,v 1.13 2010/02/28 08:36:36 otto Exp $

.if (${MACHINE} == "amd64") || (${MACHINE} == "i386")

SRCS=	apm.c apmsubr.c

.PATH:	${.CURDIR}/../apmd

CFLAGS+=	-I${.CURDIR}/../apmd
PROG=	apm
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.else
NOPROG=yes
.endif

MAN=	apm.8
MANSUBDIR=amd64 i386
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
