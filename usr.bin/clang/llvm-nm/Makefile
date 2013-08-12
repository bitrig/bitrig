# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-nm

SRCDIR=	tools/llvm-nm
SRCS=	llvm-nm.cpp

LIBDEPS=llvmobject \
	llvmarchive \
	llvmbitreader \
	llvmcore \
	llvmsupport

.include "../clang.prog.mk"
