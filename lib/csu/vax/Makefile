#	$OpenBSD: Makefile,v 1.10 2013/12/03 06:21:41 guenther Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 5/22/91

CFLAGS+=	-I${.CURDIR}/..
OBJS=		crt0.o gcrt0.o scrt0.o
CLEANFILES+=	core crt0.out gcrt0.out scrt0.out

all: ${OBJS}

crt0.o: crt0.c
	@echo ${COMPILE.c} -DCRT0 ${ELFDIR}/crt0.c -o ${.TARGET}
	@${COMPILE.c} -DCRT0 ${ELFDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo ${COMPILE.c} -DMCRT0 ${ELFDIR}/crt0.c -o ${.TARGET}
	@${COMPILE.c} -DMCRT0 ${ELFDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbegin.o: crtbegin.c
	@echo ${COMPILE.c} ${ELFDIR}/crtbegin.c -o ${.TARGET}
	@${COMPILE.c} ${ELFDIR}/crtbegin.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbeginS.o: crtbeginS.c
	@echo ${COMPILE.c} ${PICFLAG} ${ELFDIR}/crtbeginS.c -o ${.TARGET}
	@${COMPILE.c} ${PICFLAG} ${ELFDIR}/crtbeginS.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo ${COMPILE.c} ${ELFDIR}/crtend.c -o ${.TARGET}
	@${COMPILE.c} ${ELFDIR}/crtend.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtendS.o: crtendS.c
	@echo ${COMPILE.c} ${PICFLAG} ${ELFDIR}/crtendS.c -o ${.TARGET}
	@${COMPILE.c} ${PICFLAG} ${ELFDIR}/crtendS.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

realinstall:
	${INSTALL} ${INSTALL_COPY} -S -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
