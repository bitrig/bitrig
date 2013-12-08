# $OpenBSD: Makefile,v 1.12 2013/12/08 09:09:48 martynas Exp $

NOPROG=
NOMAN=
CC=${HOSTCC}

FIRM=	rum-rt2573 run-rt2870 run-rt3071

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
	    ${.CURDIR}/rum-license ${DESTDIR}/etc/firmware
	ln -f ${DESTDIR}/etc/firmware/rum-license \
	    ${DESTDIR}/etc/firmware/run-license

.include <bsd.prog.mk>
