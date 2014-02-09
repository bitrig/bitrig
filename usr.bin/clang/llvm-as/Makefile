# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-as

SRCDIR=	tools/llvm-as
SRCS=	llvm-as.cpp

LIBDEPS=llvmbitwriter \
	llvmasmparser \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
