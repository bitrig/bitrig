# $FreeBSD$

.include <bsd.own.mk>

PROG=opt

SRCDIR=	tools/opt
SRCS=	AnalysisWrappers.cpp \
	BreakpointPrinter.cpp \
	GraphPrinters.cpp \
	NewPMDriver.cpp \
	PassPrinters.cpp \
	PrintSCC.cpp \
	opt.cpp

TGHDRS=	Attributes \
	Intrinsics

LIBDEPS=llvmpasses \
	llvmaarch64disassembler \
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
	llvmpowerpcdisassembler \
	llvmpowerpccodegen \
	llvmpowerpcasmparser \
	llvmpowerpcdesc \
	llvmpowerpcinfo \
	llvmpowerpcinstprinter \
	llvmx86disassembler \
	llvmx86asmparser \
	llvmx86codegen \
	llvmselectiondag \
	llvmasmprinter \
	llvmcodegen \
	llvmtarget \
	llvmx86desc \
	llvmmcdisassembler \
	llvmx86info \
	llvmx86instprinter \
	llvmx86utils \
	llvmipo \
	llvmvectorize \
	llvmlinker \
	llvmobjcarcopts \
	llvmscalaropts \
	llvminstcombine \
	llvminstrumentation \
	llvmprofiledata \
	llvmobject \
	llvmmcparser \
	llvmtransformutils \
	llvmmc \
	llvmanalysis \
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
