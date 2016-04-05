# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmpowerpccodegen

SRCDIR=	lib/Target/PowerPC
SRCS=	PPCAsmPrinter.cpp \
	PPCBranchSelector.cpp \
	PPCCTRLoops.cpp \
	PPCFastISel.cpp \
	PPCFrameLowering.cpp \
	PPCHazardRecognizers.cpp \
	PPCISelDAGToDAG.cpp \
	PPCISelLowering.cpp \
	PPCInstrInfo.cpp \
	PPCMCInstLower.cpp \
	PPCMachineFunctionInfo.cpp \
	PPCRegisterInfo.cpp \
	PPCSubtarget.cpp \
	PPCTargetMachine.cpp \
	PPCTargetObjectFile.cpp \
	PPCTargetTransformInfo.cpp \
	PPCBoolRetToInt.cpp \
	PPCEarlyReturn.cpp \
	PPCLoopDataPrefetch.cpp \
	PPCLoopPreIncPrep.cpp \
	PPCMIPeephole.cpp \
	PPCTLSDynamicCall.cpp \
	PPCTOCRegDeps.cpp \
	PPCVSXCopy.cpp \
	PPCVSXFMAMutate.cpp \
	PPCVSXSwapRemoval.cpp


TGHDRS=	Attributes \
	Intrinsics \
	PPCGenCallingConv \
	PPCGenCodeEmitter \
	PPCGenDAGISel \
	PPCGenFastISel \
	PPCGenInstrInfo \
	PPCGenMCCodeEmitter \
	PPCGenRegisterInfo \
	PPCGenSubtargetInfo

.include "../clang.lib.mk"
