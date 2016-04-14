# $FreeBSD$

.include <bsd.own.mk>

LIB=	compiler_rt
NO_PIC=
WARNS?=	2

CFLAGS+=${PICFLAG} -fvisibility=hidden -DVISIBILITY_HIDDEN

.if ${MACHINE_CPU} == "amd64"
CRTARCH=x86_64
.else
CRTARCH=${MACHINE_CPU}
.endif

CRTSRC=${.CURDIR}/../../contrib/compiler-rt/lib/builtins

.PATH: ${CRTSRC}/${CRTARCH} ${CRTSRC}

SRCF=	absvdi2 \
	absvsi2 \
	absvti2 \
	addvdi3 \
	addvsi3 \
	addvti3 \
	ashldi3 \
	ashlti3 \
	ashrdi3 \
	ashrti3 \
	clear_cache \
	clzdi2 \
	clzsi2 \
	clzti2 \
	cmpdi2 \
	cmpti2 \
	ctzdi2 \
	ctzsi2 \
	ctzti2 \
	divdc3 \
	divdi3 \
	divmoddi4 \
	divmodsi4 \
	divsc3 \
	divtc3 \
	divti3 \
	divxc3 \
	enable_execute_stack \
	eprintf \
	extendhfsf2 \
	ffsdi2 \
	ffsti2 \
	fixdfdi \
	fixdfti \
	fixsfdi \
	fixsfti \
	fixunsdfdi \
	fixunsdfsi \
	fixunsdfti \
	fixunssfdi \
	fixunssfsi \
	fixunssfti \
	fixunsxfdi \
	fixunsxfsi \
	fixunsxfti \
	fixxfdi \
	fixxfti \
	floatdidf \
	floatdisf \
	floatditf \
	floatdixf \
	floatsitf \
	floattidf \
	floattisf \
	floattixf \
	floatundidf \
	floatundisf \
	floatunditf \
	floatundixf \
	floatunsidf \
	floatunsisf \
	floatuntidf \
	floatuntisf \
	floatuntixf \
	gcc_personality_v0 \
	int_util \
	lshrdi3 \
	lshrti3 \
	moddi3 \
	modti3 \
	muldc3 \
	muldi3 \
	mulodi4 \
	mulosi4 \
	muloti4 \
	mulsc3 \
	multi3 \
	mulvdi3 \
	mulvsi3 \
	mulvti3 \
	multc3 \
	mulxc3 \
	negdf2 \
	negdi2 \
	negsf2 \
	negti2 \
	negvdi2 \
	negvsi2 \
	negvti2 \
	paritydi2 \
	paritysi2 \
	parityti2 \
	popcountdi2 \
	popcountsi2 \
	popcountti2 \
	powidf2 \
	powisf2 \
	powitf2 \
	powixf2 \
	subvdi3 \
	subvsi3 \
	subvti3 \
	trampoline_setup \
	truncdfhf2 \
	truncsfhf2 \
	ucmpdi2 \
	ucmpti2 \
	udivdi3 \
	udivmoddi4 \
	udivmodsi4 \
	udivmodti4 \
	udivti3 \
	umoddi3 \
	umodti3

# These are already shipped by libc.a on arm and mips
.if ${MACHINE_CPU} != "arm" && ${MACHINE_CPU} != "mips"
SRCF+=	adddf3 \
	addsf3 \
	divdf3 \
	divsf3 \
	extendsfdf2 \
	fixdfsi \
	fixsfsi \
	floatsidf \
	floatsisf \
	muldf3 \
	mulsf3 \
	subdf3 \
	subsf3 \
	truncdfsf2
.endif

.if ${MACHINE_CPU} != "arm"
SRCF+=	comparedf2 \
	comparesf2
.endif

.if ${MACHINE_CPU} != "mips" && \
    (${MACHINE_CPU} != "arm" || ${MK_ARM_EABI:L} != "no")
SRCF+=	divsi3 \
	modsi3 \
	udivsi3 \
	umodsi3
.endif

# FreeBSD-specific atomic intrinsics.
.if ${MACHINE_CPU} == "arm"
.PATH: ${.CURDIR}/../../sys/arm/arm

#SRCF+=	stdatomic
.elif ${MACHINE_CPU} == "mips"
.PATH: ${.CURDIR}/../../sys/mips/mips

SRCF+=	stdatomic
.endif

.for file in ${SRCF}
. if ${MACHINE_CPU} != "arm" && exists(${CRTSRC}/${CRTARCH}/${file}.S)
SRCS+=	${file}.S
. else
SRCS+=	${file}.c
. endif
.endfor

.if ${MACHINE_CPU} == "arm" && ${MK_ARM_EABI:L} != "no"
SRCS+=	aeabi_div0.c \
	aeabi_idivmod.S \
	aeabi_ldivmod.S \
	aeabi_memcmp.S \
	aeabi_memcpy.S \
	aeabi_memmove.S \
	aeabi_memset.S \
	aeabi_uidivmod.S \
	aeabi_uldivmod.S \
	bswapdi2.S \
	bswapsi2.S \
	switch16.S \
	switch32.S \
	switch8.S \
	switchu8.S \
	sync_synchronize.S
.endif

.if ${MACHINE_CPU} == "aarch64"
SRCS+=	addtf3.c \
	comparetf2.c \
	divtf3.c \
	extenddftf2.c \
	extendsftf2.c \
	fixtfsi.c \
	multf3.c \
	subtf3.c \
	trunctfdf2.c \
	trunctfsf2.c

.endif

LINKS+=${LIBDIR}/libcompiler_rt.a ${LIBDIR}/libclang_rt.${MACHINE_CPU}.a
.if ${MK_INSTALLLIB:L} != "no"
SYMLINKS+=libcompiler_rt.a ${LIBDIR}/libgcc.a
.endif
.if !defined(NOPROFILE)
SYMLINKS+=libcompiler_rt_p.a ${LIBDIR}/libgcc_p.a
.endif

.if ${MACHINE_CPU} == "amd64" || ${MACHINE_CPU} == "i386" || \
    ${MACHINE_CPU} == "powerpc"
## not supporte by Bitrig clang (!?!?!)
#AFLAGS+=--noexecstack
ACFLAGS+=-Wa,--noexecstack
.endif


.include <bsd.lib.mk>
