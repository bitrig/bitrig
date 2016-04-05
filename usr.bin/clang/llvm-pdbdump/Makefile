# $FreeBSD$

NOMAN=
.include <bsd.own.mk>

PROG=llvm-pdbdump
DPADD=	${LIBZ}
LDADD=	-lz

SRCDIR=	tools/llvm-pdbdump
SRCS=	BuiltinDumper.cpp \
	ClassDefinitionDumper.cpp \
	CompilandDumper.cpp \
	EnumDumper.cpp \
	ExternalSymbolDumper.cpp \
	FunctionDumper.cpp \
	LinePrinter.cpp \
	TypeDumper.cpp \
	TypedefDumper.cpp \
	VariableDumper.cpp \
	llvm-pdbdump.cpp

LIBDEPS=llvmdebuginfopdb \
	llvmobject \
	llvmmcparser \
	llvmmc \
	llvmbitreader \
	llvmcore \
	llvmsupport


DPADD=  ${LIBCURSES} ${LIBZ}
LDADD=  -lcurses -lz

.include "../clang.prog.mk"
