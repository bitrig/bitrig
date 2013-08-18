#	$OpenBSD: Makefile,v 1.6 2001/07/20 19:44:03 mickey Exp $
#	from: @(#)Makefile	6.13 (Berkeley) 3/20/91

IFILES=	tmac.andoc
LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
NOOBJ=	noobj

SUBDIR=	mdoc

all clean cleandir depend lint tags:

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${IFILES} \
		${DESTDIR}${BINDIR}/tmac

.include <bsd.prog.mk>
