#	$OpenBSD: Makefile.inc,v 1.4 2012/09/02 16:32:52 jsg Exp $

CFLAGS += -fpic -fno-stack-protector -DRTLD_NO_WXORX -DTLS_DESC
CFLAGS += -mcpu=cortex-a57+nofp+nosimd # this is -msoft-float 
AFLAGS += -D_STANDALONE
CPPFLAGS += -I${.CURDIR}/../../lib/libc/arch/aarch64
#SRCS+= divsi3.S
#SRCS+= modsi3.S
#SRCS+= udivsi3.S
#SRCS+= umodsi3.S
#SRCS+= aeabi_div0.c
ELF_LDFLAGS+=-z nocombreloc
LIBCOMPILER_RTDIR=${.CURDIR}/../../contrib/compiler-rt
.PATH: ${LIBCOMPILER_RTDIR}/lib/builtins/aarch64
