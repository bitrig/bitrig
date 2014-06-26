#	$OpenBSD: Makefile.inc,v 1.4 2012/09/02 16:32:52 jsg Exp $

CFLAGS += -fpic -msoft-float -fno-stack-protector -DRTLD_NO_WXORX
AFLAGS += -D_STANDALONE
CPPFLAGS += -I${.CURDIR}/../../lib/libc/arch/arm
SRCS+= divsi3.S
SRCS+= modsi3.S
SRCS+= udivsi3.S
SRCS+= umodsi3.S
ELF_LDFLAGS+=-z nocombreloc
LIBCOMPILER_RTDIR=${.CURDIR}/../../contrib/compiler-rt
.PATH: ${LIBCOMPILER_RTDIR}/lib/arm
