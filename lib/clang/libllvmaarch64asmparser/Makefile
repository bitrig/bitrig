# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmaarch64asmparser

SRCDIR=	lib/Target/AArch64/AsmParser
INCDIR= lib/Target/AArch64
SRCS=	AArch64AsmParser.cpp

TGHDRS=	AArch64GenInstrInfo \
	AArch64GenRegisterInfo \
	AArch64GenSubtargetInfo \
	AArch64GenAsmMatcher

.include "../clang.lib.mk"
