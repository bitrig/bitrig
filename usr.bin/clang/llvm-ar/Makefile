# $FreeBSD$

.include <bsd.own.mk>

PROG=llvm-ar

SRCDIR=	tools/llvm-ar
SRCS=	llvm-ar.cpp

LIBDEPS=llvmaarch64asmparser \
	llvmaarch64desc \
	llvmaarch64info \
	llvmaarch64instprinter \
	llvmaarch64utils \
	llvmarmasmparser \
	llvmarmdesc \
	llvmarminfo \
	llvmarminstprinter \
	llvmmipsasmparser \
	llvmmipsdesc \
	llvmmipsinfo \
	llvmmipsinstprinter \
	llvmpowerpcdesc \
	llvmpowerpcinfo \
	llvmpowerpcinstprinter \
	llvmx86asmparser \
	llvmx86codegen \
	llvmx86desc \
	llvmx86info \
	llvmx86instprinter \
	llvmx86utils \
	llvmcodegen \
	llvmmcdisassembler \
	llvmtransformutils \
	llvmanalysis \
	llvmtarget \
	llvmobject \
	llvmmc \
	llvmmcparser \
	llvmbitreader \
	llvmcore \
	llvmsupport

DPADD=  ${LIBCURSES} ${LIBZ}
LDADD=  -lcurses
LDADD+= -lz

.include "../clang.prog.mk"
