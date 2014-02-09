# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-nm

SRCDIR=	tools/llvm-nm
SRCS=	llvm-nm.cpp

LIBDEPS=llvmobject \
	llvmbitreader \
	llvmcore \
	llvmobject \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses


.include "../clang.prog.mk"
