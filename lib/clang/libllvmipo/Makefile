# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmipo

SRCDIR=	lib/Transforms/IPO
SRCS=	ArgumentPromotion.cpp \
	BarrierNoopPass.cpp \
	ConstantMerge.cpp \
	DeadArgumentElimination.cpp \
	ExtractGV.cpp \
	FunctionAttrs.cpp \
	GlobalDCE.cpp \
	GlobalOpt.cpp \
	IPConstantPropagation.cpp \
	InlineAlways.cpp \
	InlineSimple.cpp \
	Inliner.cpp \
	Internalize.cpp \
	IPO.cpp \
	LoopExtractor.cpp \
	MergeFunctions.cpp \
	PartialInlining.cpp \
	PassManagerBuilder.cpp \
	PruneEH.cpp \
	StripDeadPrototypes.cpp \
	StripSymbols.cpp \
	CrossDSOCFI.cpp \
	ElimAvailExtern.cpp \
	ForceFunctionAttrs.cpp \
	FunctionImport.cpp \
	InferFunctionAttrs.cpp \
	LowerBitSets.cpp \
	SampleProfile.cpp


TGHDRS=	Attributes \
	Intrinsics

.include "../clang.lib.mk"
