# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmbitwriter

SRCDIR=	lib/Bitcode/Writer
SRCS=	BitWriter.cpp \
	BitcodeWriter.cpp \
	BitcodeWriterPass.cpp \
	ValueEnumerator.cpp

TGHDRS=	Attributes \
	Intrinsics

.include "../clang.lib.mk"
