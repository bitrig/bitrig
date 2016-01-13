# $FreeBSD$

.include <bsd.own.mk>

SRCS+=	aeabi_atexit.c		\
	aeabi_unwind_cpp.c	

.PATH: ${LIBCSRCDIR}/arch/arm/aeabi

# Add the aeabi_mem* functions. While they live in compiler-rt they call into
# libc. This causes issues when other parts of libc call these functions.
# We work around this by including these functions in libc but mark them as
# hidden so users of libc will not pick up these versions.

#SRCS+=	aeabi_memcmp.S		\
#	aeabi_memcpy.S		\
#	aeabi_memmove.S		\
#	aeabi_memset.S

SRCS+=	multf3.c extenddftf2.c addtf3.c comparetf2.c extendsftf2.c
SRCS+=	subtf3.c extendsftf2.c trunctfsf2.c trunctfdf2.c
SRCS+=	divtf3.c
SRCS+=  fixtfsi.c floatsitf.c floatditf.c

.PATH: ${LIBCSRCDIR}/../../contrib/compiler-rt/lib/builtins

# FIXME
# Mark the functions as hidden so they are not available outside of libc.
#CFLAGS.aeabi_memcmp.S=	-DVISIBILITY_HIDDEN
#CFLAGS.aeabi_memcpy.S=	-DVISIBILITY_HIDDEN
#CFLAGS.aeabi_memmove.S=	-DVISIBILITY_HIDDEN
#CFLAGS.aeabi_memset.S=	-DVISIBILITY_HIDDEN
#CFLAGS+=		${CFLAGS.${.IMPSRC:T}}


