# $FreeBSD$

NOMAN=
.include <bsd.own.mk>

PROG=llvm-rtdyld
DPADD=	${LIBZ}
LDADD=	-lz

SRCDIR=	tools/llvm-rtdyld
SRCS=	llvm-rtdyld.cpp

LIBDEPS=llvmdebuginfo \
	llvmexecutionengine \
	llvmruntimedyld \
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
	llvmpowerpccodegen \
	llvmpowerpcdesc \
	llvmpowerpcinfo \
	llvmpowerpcinstprinter \
	llvmx86asmparser \
	llvmx86codegen \
	llvmx86desc \
	llvmx86disassembler \
	llvmx86info \
	llvmx86instprinter \
	llvmx86utils \
	llvmselectiondag \
	llvmasmprinter \
	llvmcodegen \
	llvmmcdisassembler \
	llvmobjcarcopts \
	llvmscalaropts \
	llvminstcombine \
	llvmtransformutils \
	llvmipa \
	llvmobject \
	llvmmcparser \
	llvmanalysis \
	llvmbitreader \
	llvmtarget \
	llvmmc \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES} ${LIBZ}
LDADD=  -lcurses -lz

.include "../clang.prog.mk"
