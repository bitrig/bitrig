#	$OpenBSD: Makefile,v 1.10 2010/05/10 18:20:31 drahn Exp $

.include <bsd.own.mk>

NOMAN=

PROG=cpp
SRCS=
INSTALL_STRIP=

cpp: cpp.sh
.if ${COMPILER_VERSION:L} == "gcc4"
	sed -e 's/@GNUC@//' -e 's/@dollaropt@//' ${.CURDIR}/cpp.sh >$@
.elif ${COMPILER_VERSION:L} == "clang"
	sed -e 's/@GNUC@//' -e 's/@dollaropt@//' ${.CURDIR}/clang_cpp.sh >$@
.else
	@echo invalid compiler
	@exit 1
.endif

.include <bsd.prog.mk>
