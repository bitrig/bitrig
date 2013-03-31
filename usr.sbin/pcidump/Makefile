#	$OpenBSD: Makefile,v 1.6 2013/03/31 16:41:45 deraadt Exp $

.if ${MACHINE} == "amd64" ||  ${MACHINE} == "armish" || \
    ${MACHINE} == "i386"

PROG=	pcidump

.else
NOPROG=yes
.endif

MAN=	pcidump.8

.include <bsd.prog.mk>
