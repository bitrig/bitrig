#	$OpenBSD: Makefile,v 1.16 2011/11/21 01:36:18 jsg Exp $

.include <bsd.own.mk>

SUBDIR+=libiberty libreadline libstdc++-v3 libsupc++-v3
.if make(obj)
SUBDIR+=libiberty libreadline libstdc++-v3 libsupc++-v3
.else
.  if ${COMPILER_VERSION:L} == "gcc4"
SUBDIR+=../usr.bin/cc/libobjc
.  elif ${COMPILER_VERSION:L} == "clang"
# make this work with clang
#SUBDIR+=../usr.bin/cc/libobjc
.  endif
.endif

.include <bsd.subdir.mk>
