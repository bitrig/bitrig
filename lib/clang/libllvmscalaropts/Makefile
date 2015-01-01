# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmscalaropts

SRCDIR=	lib/Transforms/Scalar
SRCS=	ADCE.cpp \
	AlignmentFromAssumptions.cpp \
	ConstantHoisting.cpp \
	ConstantProp.cpp \
	CorrelatedValuePropagation.cpp \
	DCE.cpp \
	DeadStoreElimination.cpp \
	EarlyCSE.cpp \
	FlattenCFGPass.cpp \
	GVN.cpp \
	IndVarSimplify.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoadCombine.cpp \
	LoopDeletion.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopRerollPass.cpp \
	LoopRotation.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LowerAtomic.cpp \
	MemCpyOptimizer.cpp \
	MergedLoadStoreMotion.cpp \
	PartiallyInlineLibCalls.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	SCCP.cpp \
	SROA.cpp \
	SampleProfile.cpp \
	Scalar.cpp \
	Scalarizer.cpp \
	ScalarReplAggregates.cpp \
	SeparateConstOffsetFromGEP.cpp \
	SimplifyCFGPass.cpp \
	Sink.cpp \
	StructurizeCFG.cpp \
	TailRecursionElimination.cpp

.if ${MK_CLANG_EXTRAS:L} != "no"
SRCS+=	LoopInstSimplify.cpp \
	LowerAtomic.cpp \
	Reg2Mem.cpp \
	Scalar.cpp
.endif

TGHDRS=	Intrinsics

.include "../clang.lib.mk"
