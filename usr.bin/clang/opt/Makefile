# $FreeBSD$

.include <bsd.own.mk>

PROG=opt

SRCDIR=	tools/opt
SRCS=	AnalysisWrappers.cpp \
	BreakpointPrinter.cpp \
	GraphPrinters.cpp \
	NewPMDriver.cpp \
	PassPrinters.cpp \
	Passes.cpp \
	PrintSCC.cpp \
	opt.cpp

TGHDRS=	Intrinsics
LIBDEPS=llvmaarch64disassembler \
	llvmaarch64codegen \
	llvmaarch64asmparser \
	llvmaarch64desc \
	llvmaarch64info \
	llvmaarch64instprinter \
	llvmaarch64utils \
	llvmarmdisassembler \
	llvmarmcodegen \
	llvmarmasmparser \
	llvmarmdesc \
	llvmarminfo \
	llvmarminstprinter \
	llvmmipsdisassembler \
	llvmmipscodegen \
	llvmmipsasmparser \
	llvmmipsdesc \
	llvmmipsinfo \
	llvmmipsinstprinter \
	llvmpowerpccodegen \
	llvmpowerpcdesc \
	llvmpowerpcinfo \
	llvmpowerpcinstprinter \
	llvmx86disassembler \
	llvmx86asmparser \
	llvmx86codegen \
	llvmselectiondag \
	llvmasmprinter \
	llvmmcdisassembler \
	llvmmcparser \
	llvmcodegen \
	llvmx86desc \
	llvmx86info \
	llvmx86instprinter \
	llvmx86utils \
	llvmipo \
	llvmvectorize \
	llvmobjcarcopts \
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
	llvmirreader \
	llvmasmparser \
	llvmbitwriter \
	llvmbitreader \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES} ${LIBZ}
LDADD=  -lcurses
LDADD+= -lz

.include "../clang.prog.mk"
