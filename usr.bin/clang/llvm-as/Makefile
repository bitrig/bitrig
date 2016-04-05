# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-as

SRCDIR=	tools/llvm-as
SRCS=	llvm-as.cpp

TGHDRS= Attributes \
	Intrinsics

LIBDEPS=llvmbitwriter \
	llvmasmparser \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
