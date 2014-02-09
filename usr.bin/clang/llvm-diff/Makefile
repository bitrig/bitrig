# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-diff

SRCDIR=	tools/llvm-diff
SRCS=	llvm-diff.cpp \
	DiffConsumer.cpp \
	DiffLog.cpp \
	DifferenceEngine.cpp

LIBDEPS=llvmirreader \
	llvmbitreader \
	llvmasmparser \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
