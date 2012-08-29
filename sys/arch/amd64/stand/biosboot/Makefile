#	$OpenBSD: Makefile,v 1.5 2012/08/29 17:45:00 pascal Exp $

.include <bsd.own.mk>

MAN=	biosboot.8

.if ${MACHINE} == "amd64"
PROG=	biosboot
SRCS=	biosboot.S
LD=ld
LDFLAGS=-melf_i386 -nostdlib -Ttext 0 -N -x -Bstatic -nopie
LDFLAGS+=-L/usr/libdata 
INSTALL_STRIP=
SADIR=	${.CURDIR}/..

${PROG}: $(OBJS) $(DPADD)
	@rm -f $(PROG)
	$(LD) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDADD)
	@size $(PROG)

CPPFLAGS+=-DLOADADDR=$(LOADADDR) -DLINKADDR=$(LINKADDR) -DBOOTMAGIC=$(BOOTMAGIC)
CPPFLAGS+=${DEBUGFLAGS}
.if ${COMPILER_VERSION:L} != "gcc4"
AFLAGS+=-no-integrated-as
.endif
CFLAGS+=-fno-pie
AFLAGS+=-m32 # -Wa,-a
AFLAGS+=-fno-pie
.else
NOPROG=
.endif

.include <bsd.prog.mk>

