# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmipa

SRCDIR=	lib/Analysis/IPA
SRCS=	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	CallPrinter.cpp \
	FindUsedTypes.cpp \
	GlobalsModRef.cpp \
	InlineCost.cpp \
	IPA.cpp

.if ${MK_CLANG_EXTRAS:L} != "no"
SRCS+=	IPA.cpp
.endif

TGHDRS=	Intrinsics

.include "../clang.lib.mk"
