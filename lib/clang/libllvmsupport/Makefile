# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvmsupport

SRCDIR=	lib/Support
SRCS=	APFloat.cpp \
	APInt.cpp \
	APSInt.cpp \
	Allocator.cpp \
	ARMBuildAttrs.cpp \
	ARMWinEH.cpp \
	Atomic.cpp \
	BlockFrequency.cpp \
	BranchProbability.cpp \
	CommandLine.cpp \
	Compression.cpp \
	ConvertUTF.c \
	ConvertUTFWrapper.cpp \
	CrashRecoveryContext.cpp \
	DAGDeltaAlgorithm.cpp \
	DataExtractor.cpp \
	DataStream.cpp \
	Debug.cpp \
	DeltaAlgorithm.cpp \
	Dwarf.cpp \
	DynamicLibrary.cpp \
	Errno.cpp \
	ErrorHandling.cpp \
	FileOutputBuffer.cpp \
	FileUtilities.cpp \
	FoldingSet.cpp \
	FormattedStream.cpp \
	GraphWriter.cpp \
	Hashing.cpp \
	Host.cpp \
	IntEqClasses.cpp \
	IntervalMap.cpp \
	IntrusiveRefCntPtr.cpp \
	IsInf.cpp \
	IsNAN.cpp \
	LEB128.cpp \
	LineIterator.cpp \
	Locale.cpp \
	LockFileManager.cpp \
	MD5.cpp \
	ManagedStatic.cpp \
	MathExtras.cpp \
	Memory.cpp \
	MemoryBuffer.cpp \
	MemoryObject.cpp \
	Mutex.cpp \
	Options.cpp \
	Path.cpp \
	PluginLoader.cpp \
	PrettyStackTrace.cpp \
	Process.cpp \
	Program.cpp \
	RWMutex.cpp \
	RandomNumberGenerator.cpp \
	Regex.cpp \
	ScaledNumber.cpp \
	SearchForAddressOfSpecialSymbol.cpp \
	Signals.cpp \
	SmallPtrSet.cpp \
	SmallVector.cpp \
	SourceMgr.cpp \
	SpecialCaseList.cpp \
	Statistic.cpp \
	StreamingMemoryObject.cpp \
	StringExtras.cpp \
	StringMap.cpp \
	StringPool.cpp \
	StringRef.cpp \
	SystemUtils.cpp \
	TargetRegistry.cpp \
	ThreadLocal.cpp \
	Threading.cpp \
	TimeValue.cpp \
	Timer.cpp \
	ToolOutputFile.cpp \
	Triple.cpp \
	Twine.cpp \
	Unicode.cpp \
	Valgrind.cpp \
	Watchdog.cpp \
	YAMLParser.cpp \
	YAMLTraits.cpp \
	circular_raw_ostream.cpp \
	raw_os_ostream.cpp \
	raw_ostream.cpp \
	regcomp.c \
	regerror.c \
	regexec.c \
	regfree.c \
	regstrlcpy.c

.include "../clang.lib.mk"

# Ugly hack to work around CLOCK_PROCESS_CPUTIME_ID not being properly defined
# between r239347 and r245428.
CXXFLAGS.Process.cpp=	-DCLOCK_PROCESS_CPUTIME_ID=15
.if !target(depend)
CXXFLAGS+=		${CXXFLAGS.${.IMPSRC:T}}
.endif
