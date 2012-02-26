# $OpenBSD: Makefile,v 1.4 2010/01/30 20:48:20 miod Exp $

NOPROG=
NOMAN=
CC=${HOSTCC}

FIRM= 	tusb3410
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
	    ${.CURDIR}/tusb3410-license ${DESTDIR}/etc/firmware
.include <bsd.prog.mk>
