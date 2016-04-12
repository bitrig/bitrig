# $FreeBSD$

HDRPATH= ${.CURDIR}/../../../contrib/llvm/tools/clang/lib/Headers
SANPATH= ${.CURDIR}/../../../contrib/compiler-rt/include/sanitizer
.PATH: ${HDRPATH}

INCSDIR=/usr/lib/clang/3.8.0/include

INCS=	__clang_cuda_runtime_wrapper.h \
	__stddef_max_align_t.h \
	__wmmintrin_aes.h \
	__wmmintrin_pclmul.h \
	adxintrin.h \
	altivec.h \
	ammintrin.h \
	arm_acle.h \
	avx2intrin.h \
	avx512bwintrin.h \
	avx512cdintrin.h \
	avx512dqintrin.h \
	avx512erintrin.h \
	avx512fintrin.h \
	avx512vlbwintrin.h \
	avx512vldqintrin.h \
	avx512vlintrin.h \
	avxintrin.h \
	bmi2intrin.h \
	bmiintrin.h \
	cpuid.h \
	cuda_builtin_vars.h \
	emmintrin.h \
	f16cintrin.h \
	fma4intrin.h \
	fmaintrin.h \
	fxsrintrin.h \
	htmintrin.h \
	htmxlintrin.h \
	ia32intrin.h \
	immintrin.h \
	lzcntintrin.h \
	mm3dnow.h \
	mm_malloc.h \
	mmintrin.h \
	module.modulemap \
	nmmintrin.h \
	pkuintrin.h \
	pmmintrin.h \
	popcntintrin.h \
	prfchwintrin.h \
	rdseedintrin.h \
	rtmintrin.h \
	s390intrin.h \
	shaintrin.h \
	smmintrin.h \
	tbmintrin.h \
	tmmintrin.h \
	unwind.h \
	vadefs.h \
	vecintrin.h \
	wmmintrin.h \
	x86intrin.h \
	xmmintrin.h \
	xopintrin.h \
	xsavecintrin.h \
	xsaveintrin.h \
	xsaveoptintrin.h \
	xsavesintrin.h \
	xtestintrin.h \

GENINCS= arm_neon.h

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
