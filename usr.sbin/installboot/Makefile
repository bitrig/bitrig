#	$OpenBSD: Makefile,v 1.8 2014/01/05 16:02:40 jsing Exp $

.if (${MACHINE} == "amd64" || ${MACHINE} == "i386")

PROG=		installboot
SRCS=		installboot.c
MAN=		installboot.8

CPPFLAGS=	-I${.CURDIR}
LDADD=		-lutil
DPADD=		${LIBUTIL}

LDSTATIC=	-static

.include "${.CURDIR}/${MACHINE}/Makefile.inc"

.if !empty(CFLAGS:M-DBOOTSTRAP)
SRCS += bootstrap.c
.endif

.if !empty(CFLAGS:M-DSOFTRAID)
SRCS += softraid.c
.endif

.else
NOPROG=
NOMAN=
.endif

.include <bsd.prog.mk>
