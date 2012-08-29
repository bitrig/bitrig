#	$OpenBSD: Makefile,v 1.23 2012/08/29 22:23:07 pascal Exp $

CC=gcc

MAN=	biosboot.8

.if ${MACHINE} == "i386"
PROG=	biosboot
SRCS=	biosboot.S
CFLAGS= -no-builtin-asm
LD=ld
LDFLAGS=-nostdlib -Ttext 0 -N -x -Bstatic -nopie
INSTALL_STRIP=
SADIR=	${.CURDIR}/..

${PROG}: $(OBJS) $(DPADD)
	@rm -f $(PROG)
	$(LD) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDADD)
	@size $(PROG)

CPPFLAGS+=-DLOADADDR=$(LOADADDR) -DLINKADDR=$(LINKADDR) -DBOOTMAGIC=$(BOOTMAGIC)
CPPFLAGS+=${DEBUGFLAGS}
CFLAGS+=-fno-pie
#AFLAGS+=-Wa,-a
AFLAGS+=-fno-pie
.else
NOPROG=
.endif

.include <bsd.prog.mk>

