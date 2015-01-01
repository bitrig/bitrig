# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmaarch64utils

SRCDIR=	lib/Target/AArch64/Utils
SRCS= \
	AArch64BaseInfo.cpp \

CFLAGS+=-I${LLVM_SRCS}/lib/Target/AArch64

TGHDRS=	AArch64GenInstrInfo \
	AArch64GenRegisterInfo \
	AArch64GenSubtargetInfo

.include "../clang.lib.mk"
