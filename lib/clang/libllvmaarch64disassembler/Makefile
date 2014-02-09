# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmaarch64disassembler

SRCDIR=	lib/Target/AArch64/Disassembler
SRCS= \
	AArch64Disassembler.cpp \

CFLAGS+= -I${LLVM_SRCS}/${SRCDIR}/..

TGHDRS= \
	AArch64GenRegisterInfo \
	AArch64GenInstrInfo \
	AArch64GenSubtargetInfo \
	AArch64GenDisassemblerTables \

.include "../clang.lib.mk"
