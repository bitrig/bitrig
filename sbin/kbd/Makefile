#	$OpenBSD: Makefile,v 1.33 2009/10/11 12:19:49 maja Exp $

#
# Be sure to change src/distrib/special/kbd/Makefile as well if you add or
# remove architectures on this list!
#
PROG=	kbd

SRCS=	main.c kbd_wscons.c
LDADD=	-lkvm
DPADD=	${LIBKVM}

MAN=		kbd.8
MANSUBDIR=	amd64 i386 macppc socppc zaurus

.include <bsd.prog.mk>
