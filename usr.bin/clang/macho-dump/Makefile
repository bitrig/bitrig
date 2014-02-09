# $FreeBSD$

NOMAN=
.include <bsd.own.mk>

PROG=macho-dump

SRCDIR=	tools/macho-dump
SRCS=	macho-dump.cpp

LIBDEPS=llvmobject \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
