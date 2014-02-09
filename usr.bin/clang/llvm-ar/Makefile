# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-ar

SRCDIR=	tools/llvm-ar
SRCS=	llvm-ar.cpp

LIBDEPS=llvmbitreader \
	llvmcore \
	llvmobject \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
