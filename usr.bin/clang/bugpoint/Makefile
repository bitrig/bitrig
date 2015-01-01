# $FreeBSD$

.include <bsd.own.mk>

PROG=bugpoint

SRCDIR=	tools/bugpoint
SRCS=	BugDriver.cpp \
	CrashDebugger.cpp \
	ExecutionDriver.cpp \
	ExtractFunction.cpp \
	FindBugs.cpp \
	Miscompilation.cpp \
	OptimizerDriver.cpp \
	ToolRunner.cpp \
	bugpoint.cpp

TGHDRS=	Intrinsics
LIBDEPS=llvmcodegen \
	llvmirreader \
	llvmbitwriter \
	llvmbitreader \
	llvmlinker \
	llvmipo \
	llvmobjcarcopts \
	llvmvectorize \
	llvmscalaropts \
	llvmprofiledata \
	llvminstcombine \
	llvminstrumentation \
	llvmtransformutils \
	llvmipa \
	llvmanalysis \
	llvmtarget \
	llvmmc \
	llvmobject \
	llvmasmparser \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include "../clang.prog.mk"
