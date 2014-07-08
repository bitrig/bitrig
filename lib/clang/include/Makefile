# $FreeBSD$

HDRPATH= ${.CURDIR}/../../../contrib/llvm/tools/clang/lib/Headers
SANPATH= ${.CURDIR}/../../../contrib/compiler-rt/include/sanitizer
.PATH: ${HDRPATH}

INCSDIR=/usr/lib/clang/3.4/include

INCS=	__wmmintrin_aes.h \
	__wmmintrin_pclmul.h \
	Intrin.h \
	altivec.h \
	ammintrin.h \
	avx2intrin.h \
	avxintrin.h \
	bmi2intrin.h \
	bmiintrin.h \
	cpuid.h \
	emmintrin.h \
	f16cintrin.h \
	float.h \
	fma4intrin.h \
	fmaintrin.h \
	immintrin.h \
	iso646.h \
	limits.h \
	lzcntintrin.h \
	mm3dnow.h \
	mm_malloc.h \
	mmintrin.h \
	module.map \
	nmmintrin.h \
	pmmintrin.h \
	popcntintrin.h \
	prfchwintrin.h \
	rdseedintrin.h \
	rtmintrin.h \
	shaintrin.h \
	smmintrin.h \
	stdalign.h \
	stdarg.h \
	stdbool.h \
	stddef.h \
	stdint.h \
	stdnoreturn.h \
	tbmintrin.h \
	tgmath.h \
	tmmintrin.h \
	unwind.h \
	varargs.h \
	wmmintrin.h \
	x86intrin.h \
	xmmintrin.h \
	xopintrin.h

GENINCS= \
	arm_neon.h

SANINCS= \
	asan_interface.h \
	common_interface_defs.h \
	dfsan_interface.h \
	linux_syscall_hooks.h \
	lsan_interface.h \
	msan_interface.h

all: ${GENINCS}
includes: ${GENINCS}

LLVMDIR=${.CURDIR}/../../../contrib/llvm

CLEANFILES+= arm_neon.h

arm_neon.h:
	clang-tblgen -gen-arm-neon -I ${LLVMDIR}/tools/clang/lib/Headers -I ${LLVMDIR}/lib/Target -I ${LLVMDIR}/include ${LLVMDIR}/tools/clang/include/clang/Basic/arm_neon.td -o arm_neon.h

includes:
.for inc in ${INCS}
	cmp -s ${.OBJDIR}/${inc}.copy ${DESTDIR}${INCSDIR}/${inc} || \
	    ${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${HDRPATH}/${inc} ${DESTDIR}${INCSDIR}/${inc}
.endfor
.for inc in ${GENINCS}
	cmp -s ${.OBJDIR}/${inc}.copy ${DESTDIR}${INCSDIR}/${inc} || \
	    ${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${.OBJDIR}/${inc} ${DESTDIR}${INCSDIR}/${inc}
.endfor
.for inc in ${SANINCS}
	cmp -s ${.OBJDIR}/${inc}.copy ${DESTDIR}${INCSDIR}/${inc} || \
	    ${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${SANPATH}/${inc} ${DESTDIR}${INCSDIR}/sanitizer/${inc}
.endfor

.include <bsd.prog.mk>
