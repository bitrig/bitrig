#	$OpenBSD: Makefile,v 1.9 2014/02/21 18:30:15 deraadt Exp $

CFLAGS+=-DINET6

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c cookie.c domacro.c fetch.c ftp.c \
	list.c main.c ruserpass.c small.c stringlist.c util.c
.PATH:	${.CURDIR}/../../../usr.bin/ftp

LDADD+=	-ledit -lcurses -lutil -ltls -lssl -lcrypto
DPADD+=	${LIBEDIT} ${LIBCURSES} ${LIBUTIL} ${LIBTLS} ${LIBSSL} ${LIBCRYPTO}

.include <bsd.prog.mk>
