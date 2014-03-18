#	$OpenBSD: Makefile,v 1.35 2014/03/18 22:36:30 miod Exp $

#
# Be sure to change src/distrib/special/kbd/Makefile as well if you add or
# remove architectures on this list!
#
PROG=	kbd

SRCS=	main.c kbd_wscons.c
LDADD=	-lkvm
DPADD=	${LIBKVM}

MAN=		kbd.8
MANSUBDIR=	amd64 i386

.include <bsd.prog.mk>
