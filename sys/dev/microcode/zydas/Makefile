# $OpenBSD: Makefile,v 1.9 2010/01/30 20:48:21 miod Exp $

NOPROG=
NOMAN=
CC=${HOSTCC}

FIRM=	zd1211 zd1211b
PROG=	build

CLEANFILES+= ${FIRM} ${PROG}

all: ${FIRM}

${FIRM}: build
	${.OBJDIR}/build

realinstall:

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${FIRM} ${DESTDIR}/etc/firmware
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/zd1211-license ${DESTDIR}/etc/firmware

.include <bsd.prog.mk>
