# $OpenBSD: Makefile.inc,v 1.15 2012/04/19 19:14:56 deraadt Exp $
# $NetBSD: Makefile.inc,v 1.6 2003/08/01 17:03:47 lukem Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S fabs.c flt_rounds.c infinity.c
SRCS+=	ldexp.c modf.c nan.c
SRCS+=	setjmp.S _setjmp.S sigsetjmp.S __aeabi_read_tp.S
SRCS+=	clz_tab.c

.if ${MK_ARM_EABI:L} == "no"
SRCS+=	divsi3.S
.else
SRCS+=	fpgetmask_vfp.c fpgetround_vfp.c fpgetsticky_vfp.c \
	fpsetmask_vfp.c fpsetround_vfp.c fpsetsticky_vfp.c
.endif
