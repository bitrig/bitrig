# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmaarch64info

SRCDIR=	lib/Target/AArch64/TargetInfo
SRCS= \
	AArch64TargetInfo.cpp \

CFLAGS+= -I${LLVM_SRCS}/${SRCDIR}/..

TGHDRS= \
	AArch64GenRegisterInfo \
	AArch64GenInstrInfo \
	AArch64GenSubtargetInfo \

.include "../clang.lib.mk"
