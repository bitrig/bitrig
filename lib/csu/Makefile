#	$OpenBSD: Makefile,v 1.20 2014/11/15 21:59:29 guenther Exp $

OBJS=		crt0.o gcrt0.o
OBJS+=		crtbegin.o crtend.o
OBJS+=		crtbeginS.o crtendS.o
OBJS+=		rcrt0.o
SRCS=		crt0.c crtbegin.c crtbeginS.c crtend.c crtendS.c

CFLAGS+=	-I${.CURDIR}/${MACHINE_CPU}
CFLAGS+=	-I${.CURDIR}/../../libexec/ld.so
CFLAGS+=	-I${.CURDIR}/../../libexec/ld.so/${MACHINE_CPU}

# XXX "use -fno-omit-frame-pointer; the reason is almost crazy; pr#287"
.if ${MACHINE_ARCH} == "amd64"
CFLAGS+=	-fno-omit-frame-pointer
.endif

# XXX arm doesn't do pie yet???
.if ${MACHINE_ARCH} == "arm"
CFLAGS+=	-fpie
.endif

CRTFLAG=${NOPIE_FLAGS}
RCRTFLAG=-DRCRT0 -fno-stack-protector

all: ${OBJS}

crt0.o: crt0.c
	@echo ${COMPILE.c} ${CRTFLAG} ${.CURDIR}/crt0.c -o ${.TARGET}
	@${COMPILE.c} ${.CURDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo ${COMPILE.c} -DMCRT0 ${.CURDIR}/crt0.c -o ${.TARGET}
	@${COMPILE.c} -DMCRT0 ${.CURDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbegin.o: crtbegin.c
	@echo ${COMPILE.c} ${.CURDIR}/crtbegin.c -o ${.TARGET}
	@${COMPILE.c} ${.CURDIR}/crtbegin.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbeginS.o: crtbeginS.c
	@echo ${COMPILE.c} ${PICFLAG} ${.CURDIR}/crtbeginS.c -o ${.TARGET}
	@${COMPILE.c} ${PICFLAG} ${.CURDIR}/crtbeginS.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo ${COMPILE.c} ${.CURDIR}/crtend.c -o ${.TARGET}
	@${COMPILE.c} ${.CURDIR}/crtend.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtendS.o: crtendS.c
	@echo ${COMPILE.c} ${PICFLAG} ${.CURDIR}/crtendS.c -o ${.TARGET}
	@${COMPILE.c} ${PICFLAG} ${.CURDIR}/crtendS.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

rcrt0.o: crt0.c
	@echo ${COMPILE.c} ${RCRTFLAG} ${.CURDIR}/crt0.c -o ${.TARGET}
	@${COMPILE.c} ${RCRTFLAG} ${.CURDIR}/crt0.c -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

realinstall:
	${INSTALL} ${INSTALL_COPY} -S -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o r\1.o:/' \
	    < .depend > $$TMP; \
	mv $$TMP .depend)

.include <bsd.prog.mk>
