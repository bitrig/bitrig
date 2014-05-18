# $FreeBSD$

.PATH: ${.CURDIR}/../../../contrib/llvm/tools/clang/lib/Headers

INCSDIR=/usr/lib/clang/3.4/include

INCS=	__wmmintrin_aes.h \
	__wmmintrin_pclmul.h \
	altivec.h \
	ammintrin.h \
	arm_neon.h \
	avx2intrin.h \
	avxintrin.h \
	bmi2intrin.h \
	bmiintrin.h \
	cpuid.h \
	emmintrin.h \
	f16cintrin.h \
	fma4intrin.h \
	fmaintrin.h \
	immintrin.h \
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
	smmintrin.h \
	tmmintrin.h \
	wmmintrin.h \
	x86intrin.h \
	xmmintrin.h \
	xopintrin.h

.for inc in ${INCS}
CLEANFILES+= ${inc}.copy
all: ${inc}.copy
includes: ${inc}.copy

${inc}.copy: ${inc}
	cp ${?} ${inc}.copy
.endfor

LLVMDIR=${.CURDIR}/../../../contrib/llvm

CLEANFILES+= arm_neon.h

arm_neon.h: 
	clang-tblgen -gen-arm-neon -I ${LLVMDIR}/tools/clang/lib/Headers -I ${LLVMDIR}/lib/Target -I ${LLVMDIR}/include ${LLVMDIR}/tools/clang/include/clang/Basic/arm_neon.td -o arm_neon.h

includes:
.for inc in ${INCS}
	cmp -s ${.OBJDIR}/${inc}.copy ${DESTDIR}${INCSDIR}/${inc} || \
	    ${INSTALL} ${INSTALL_COPY} -m 444 -o $(BINOWN) -g $(BINGRP) \
	    ${.OBJDIR}/${inc}.copy ${DESTDIR}${INCSDIR}/${inc} 
.endfor

.include <bsd.prog.mk>
