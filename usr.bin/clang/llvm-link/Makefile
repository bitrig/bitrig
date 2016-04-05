# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-link

SRCDIR=	tools/llvm-link
SRCS=	llvm-link.cpp

TGHDRS=	Attributes

LIBDEPS=llvmobject \
	llvmmcparser \
	llvmmc \
	llvmirreader \
	llvmasmparser \
	llvmbitwriter \
	llvmbitreader \
	llvmlinker \
	llvmtransformutils \
	llvmanalysis \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
