# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-bcanalyzer

SRCDIR=	tools/llvm-bcanalyzer
SRCS=	llvm-bcanalyzer.cpp

TGHDRS= Attributes \
	Intrinsics

LIBDEPS=llvmbitreader \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
