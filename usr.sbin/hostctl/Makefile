#	$OpenBSD: Makefile,v 1.1 2016/01/27 12:27:31 reyk Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64" || ${MACHINE} == "arm64"

PROG=		hostctl

CFLAGS+=	-Wall

.else
NOPROG=		yes
.endif

MAN=		hostctl.8
MANSUBDIR=	amd64

.include <bsd.prog.mk>
