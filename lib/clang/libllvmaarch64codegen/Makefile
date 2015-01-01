# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmaarch64codegen

SRCDIR=	lib/Target/AArch64
SRCS=	AArch64A53Fix835769.cpp \
	AArch64A57FPLoadBalancing.cpp \
	AArch64AddressTypePromotion.cpp \
	AArch64AdvSIMDScalarPass.cpp \
	AArch64AsmPrinter.cpp \
	AArch64BranchRelaxation.cpp \
	AArch64CleanupLocalDynamicTLSPass.cpp \
	AArch64CollectLOH.cpp \
	AArch64ConditionOptimizer.cpp \
	AArch64ConditionalCompares.cpp \
	AArch64DeadRegisterDefinitionsPass.cpp \
	AArch64ExpandPseudoInsts.cpp \
	AArch64FastISel.cpp \
	AArch64FrameLowering.cpp \
	AArch64ISelDAGToDAG.cpp \
	AArch64ISelLowering.cpp \
	AArch64InstrInfo.cpp \
	AArch64LoadStoreOptimizer.cpp \
	AArch64MCInstLower.cpp \
	AArch64PromoteConstant.cpp \
	AArch64PBQPRegAlloc.cpp \
	AArch64RegisterInfo.cpp \
	AArch64SelectionDAGInfo.cpp \
	AArch64StorePairSuppress.cpp \
	AArch64Subtarget.cpp \
	AArch64TargetMachine.cpp \
	AArch64TargetObjectFile.cpp \
	AArch64TargetTransformInfo.cpp

TGHDRS=	\
	AArch64GenRegisterInfo \
	AArch64GenInstrInfo \
	AArch64GenCallingConv \
	AArch64GenFastISel \
	AArch64GenDAGISel \
	AArch64GenMCPseudoLowering \
	AArch64GenSubtargetInfo \
	Intrinsics

.include "../clang.lib.mk"
