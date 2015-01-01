# $FreeBSD$

LLVM_SRCS= ${.CURDIR}/../../../contrib/llvm

.include "../../lib/clang/clang.build.mk"

#LDADD+=-Wl,--start-group
.for lib in ${LIBDEPS}
DPADD+=	${.OBJDIR}/../../../lib/clang/lib${lib}/lib${lib}.a
LDADD+=	${.OBJDIR}/../../../lib/clang/lib${lib}/lib${lib}.a
.endfor
#LDADD+=-Wl,--end-group

BINDIR?= /usr/bin

CXXFLAGS+=${CFLAGS}

.include <bsd.prog.mk>
