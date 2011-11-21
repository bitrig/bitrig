#	$OpenBSD: Makefile,v 1.16 2011/11/21 01:36:18 jsg Exp $

.include <bsd.own.mk>

SUBDIR+=libiberty libreadline
.if make(obj)
SUBDIR+=libiberty libreadline ../usr.bin/cc/libobjc libstdc++-v3 libsupc++-v3
.else
.  if ${COMPILER_VERSION:L} == "gcc3"
SUBDIR+=libobjc libstdc++
.  elif ${COMPILER_VERSION:L} == "gcc4"
SUBDIR+=libiberty libreadline ../usr.bin/cc/libobjc libstdc++-v3 libsupc++-v3
.  elif ${COMPILER_VERSION:L} == "gcc2"
SUBDIR+=libobjc ../egcs
.  endif
.endif

.include <bsd.subdir.mk>
