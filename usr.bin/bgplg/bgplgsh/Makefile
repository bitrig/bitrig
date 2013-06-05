#	$OpenBSD: Makefile,v 1.1 2006/12/11 23:10:11 reyk Exp $

.PATH:		${.CURDIR}/..

PROG=		bgplgsh
SRCS=		bgplgsh.c misc.c
CFLAGS+=	-Wall
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CLFAGS+=	-Wmissing-declarations -Wredundant-decls
CFLAGS+=	-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+=	-Wsign-compare
# reacharound for libedit readline emulation headers.
CFLAGS+= 	-I${.CURDIR} -I${.CURDIR}/../../../lib/libedit/
LDADD=		-ledit -lncurses
MAN=		bgplgsh.8

BINDIR=		/usr/bin

.include <bsd.prog.mk>
