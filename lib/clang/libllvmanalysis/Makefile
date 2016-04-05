# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmanalysis

SRCDIR=	lib/Analysis
SRCS=	AliasAnalysis.cpp \
	AliasAnalysisEvaluator.cpp \
	AliasSetTracker.cpp \
	Analysis.cpp \
	AssumptionCache.cpp \
	BasicAliasAnalysis.cpp \
	BlockFrequencyInfo.cpp \
	BlockFrequencyInfoImpl.cpp \
	BranchProbabilityInfo.cpp \
	CFG.cpp \
	CFGPrinter.cpp \
	CFLAliasAnalysis.cpp \
	CGSCCPassManager.cpp \
	CaptureTracking.cpp \
	CodeMetrics.cpp \
	ConstantFolding.cpp \
	CostModel.cpp \
	Delinearization.cpp \
	DependenceAnalysis.cpp \
	DomPrinter.cpp \
	DominanceFrontier.cpp \
	IVUsers.cpp \
	InstCount.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
	LazyCallGraph.cpp \
	LazyValueInfo.cpp \
	Lint.cpp \
	Loads.cpp \
	LoopInfo.cpp \
	LoopPass.cpp \
	MemDepPrinter.cpp \
	MemoryBuiltins.cpp \
	MemoryDependenceAnalysis.cpp \
	ModuleDebugInfoPrinter.cpp \
	PHITransAddr.cpp \
	PostDominators.cpp \
	PtrUseVisitor.cpp \
	RegionInfo.cpp \
	RegionPass.cpp \
	RegionPrinter.cpp \
	ScalarEvolution.cpp \
	ScalarEvolutionAliasAnalysis.cpp \
	ScalarEvolutionExpander.cpp \
	ScalarEvolutionNormalization.cpp \
	ScopedNoAliasAA.cpp \
	SparsePropagation.cpp \
	TargetTransformInfo.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	ValueTracking.cpp \
	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	CallPrinter.cpp \
	DemandedBits.cpp \
	DivergenceAnalysis.cpp \
	EHPersonalities.cpp \
	GlobalsModRef.cpp \
	InlineCost.cpp \
	IteratedDominanceFrontier.cpp \
	LoopAccessAnalysis.cpp \
	MemDerefPrinter.cpp \
	MemoryLocation.cpp \
	ObjCARCAliasAnalysis.cpp \
	ObjCARCAnalysisUtils.cpp \
	ObjCARCInstKind.cpp \
	OrderedBasicBlock.cpp \
	TargetLibraryInfo.cpp \
	VectorUtils.cpp



.if ${MK_CLANG_EXTRAS:L} != "no"
SRCS+=	BlockFrequencyInfo.cpp
.endif

TGHDRS=	Attributes \
		Intrinsics

.include "../clang.lib.mk"
