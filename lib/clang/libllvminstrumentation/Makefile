# $FreeBSD$

.include <bsd.own.mk>

LIB=	llvminstrumentation

SRCDIR=	lib/Transforms/Instrumentation
SRCS=	AddressSanitizer.cpp \
	BoundsChecking.cpp \
	DataFlowSanitizer.cpp \
	GCOVProfiling.cpp \
	MemorySanitizer.cpp \
	InstrProfiling.cpp \
	Instrumentation.cpp \
	PGOInstrumentation.cpp \
	SafeStack.cpp \
	SanitizerCoverage.cpp \
	ThreadSanitizer.cpp


TGHDRS=	Attributes \
	Intrinsics

.include "../clang.lib.mk"
