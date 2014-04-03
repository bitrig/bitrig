#	$OpenBSD: Makefile,v 1.36 2014/04/03 20:45:09 miod Exp $

#
# Be sure to change src/distrib/special/kbd/Makefile as well if you add or
# remove architectures on this list!
#
PROG=	kbd

SRCS=	main.c kbd_wscons.c
LDADD=	-lkvm
DPADD=	${LIBKVM}

MAN=		kbd.8
MANSUBDIR=	amd64 i386 armv7

.include <bsd.prog.mk>
