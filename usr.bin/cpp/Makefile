#	$OpenBSD: Makefile,v 1.11 2013/08/06 19:11:53 miod Exp $

.include <bsd.own.mk>

NOMAN=

PROG=cpp
SRCS=
INSTALL_STRIP=

cpp: cpp.sh cpp_clang.sh
#.if ${COMPILER_VERSION:L} == "gcc4"
	sed -e 's/@GNUC@//' -e 's/@dollaropt@//' ${.CURDIR}/cpp.sh >$@
#.else
#	sed -e 's/@sysroot@//' ${.CURDIR}/cpp_clang.sh >$@
#.endif

.include <bsd.prog.mk>
