# $OpenBSD: Makefile,v 1.10 2010/01/30 20:48:20 miod Exp $

NOPROG=
NOMAN=
CC=${HOSTCC}

FIRM= 	kue
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
	    ${.CURDIR}/kue-license ${DESTDIR}/etc/firmware

.include <bsd.prog.mk>
