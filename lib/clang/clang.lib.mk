# $FreeBSD$

LLVM_SRCS= ${.CURDIR}/../../../contrib/llvm

.include "clang.build.mk"

CXXFLAGS+=${CFLAGS}

INTERNALLIB=

.include <bsd.lib.mk>
