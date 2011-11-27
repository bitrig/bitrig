#	$OpenBSD: Makefile,v 1.4 2010/01/30 20:48:53 miod Exp $

.if ${MACHINE} == "amd64" ||  ${MACHINE} == "armish" || \
    ${MACHINE} == "i386" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "socppc"

PROG=	pcidump

.else
NOPROG=yes
.endif

MAN=	pcidump.8

.include <bsd.prog.mk>
