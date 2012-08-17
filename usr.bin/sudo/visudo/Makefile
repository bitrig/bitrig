#	$OpenBSD: Makefile,v 1.4 2012/08/17 20:57:45 millert Exp $

.PATH:		${.CURDIR}/..

PROG=	visudo
SRCS=	pwutil.c visudo.c
BINDIR=	/usr/sbin
CPPFLAGS+=	-I.. -I${CURDIR}../lib -I${CURDIR}../../lib

MAN=	visudo.8

CLEANFILES+= ${MAN}

visudo.8: ${.CURDIR}/../varsub visudo.mdoc.in
	sed -f ${.ALLSRC} > $@

afterdepend: ${MAN}

.include <bsd.prog.mk>
