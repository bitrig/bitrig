#	$OpenBSD: Makefile,v 1.6 2003/05/06 04:32:16 tedu Exp $

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c
MAN=	newfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+=-I${MOUNT}
.PATH:	${MOUNT} ${.CURDIR}/../disklabel

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
