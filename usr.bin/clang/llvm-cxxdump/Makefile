# $FreeBSD$

NOMAN=
.include <bsd.own.mk>

PROG=llvm-cxxdump
DPADD=	${LIBZ}
LDADD=	-lz

SRCDIR=	tools/llvm-cxxdump
SRCS=	Error.cpp \
	llvm-cxxdump.cpp

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
	llvmscalaropts \
	llvminstcombine \
	llvminstrumentation \
	llvmprofiledata \
	llvmtransformutils \
	llvmbitwriter \
	llvmanalysis \
	llvmx86desc \
	llvmmcdisassembler \
	llvmx86info \
	llvmx86instprinter \
	llvmx86utils \
	llvmobject \
	llvmmcparser \
	llvmmc \
	llvmbitreader \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES} ${LIBZ}
LDADD=  -lcurses -lz

.include "../clang.prog.mk"
