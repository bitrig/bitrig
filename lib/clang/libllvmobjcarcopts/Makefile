# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmobjcarcopts

SRCDIR=	lib/Transforms/ObjCARC
SRCS=	ObjCARC.cpp \
	ObjCARCOpts.cpp \
	ObjCARCExpand.cpp \
	ObjCARCAPElim.cpp \
	ObjCARCContract.cpp \
	DependencyAnalysis.cpp \
	ProvenanceAnalysis.cpp \
	ProvenanceAnalysisEvaluator.cpp \
	PtrState.cpp

TGHDRS=	Attributes \
	Intrinsics

.include "../clang.lib.mk"
