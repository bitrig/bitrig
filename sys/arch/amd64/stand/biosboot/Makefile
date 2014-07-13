#	$OpenBSD: Makefile,v 1.10 2014/07/13 21:18:44 jasper Exp $

MAN=	biosboot.8

.if ${MACHINE} == "amd64"
S=	${.CURDIR}/../../../..
SADIR=	${.CURDIR}/..

PROG=	biosboot
SRCS=	biosboot.S
LD=ld
LDFLAGS=-nostdlib -Ttext 0 -N -x -Bstatic -nopie
LDFLAGS+=-melf_i386 -L/usr/libdata
INSTALL_STRIP=

.depend biosboot.o: assym.h

${PROG}: $(OBJS)
	@rm -f $(PROG)
	$(LD) $(LDFLAGS) -o $(PROG) $(OBJS)
	@size $(PROG)

CPPFLAGS+=-DLOADADDR=$(LOADADDR) -DLINKADDR=$(LINKADDR) -DBOOTMAGIC=$(BOOTMAGIC)
CPPFLAGS+=${DEBUGFLAGS}
CFLAGS+=-fno-pie
AFLAGS+=-m32 -Wa,--32
AFLAGS+=-fno-pie
.if ${COMPILER_VERSION:L} == "clang"
AFLAGS+=-no-integrated-as
.endif
.else
NOPROG=
.endif

.include <bsd.prog.mk>
