# $FreeBSD$

.include <bsd.own.mk>

SUBDIR=	clang clang-tblgen tblgen

.if ${MK_CLANG_EXTRAS:L} != "no" && !defined(TOOLS_PREFIX)
SUBDIR+=bugpoint \
	llc \
	lli \
	llvm-ar \
	llvm-as \
	llvm-bcanalyzer \
	llvm-diff \
	llvm-dis \
	llvm-extract \
	llvm-link \
	llvm-mc \
	llvm-nm \
	llvm-objdump \
	llvm-rtdyld \
	macho-dump \
	opt \
	scan-build
.endif

HOSTARCH!=uname -p
bootstrap:
	(cd ../../lib/clang && ${MAKE} obj)
	(cd ../../usr.bin/clang && ${MAKE} obj)
	mkdir -p /usr/lib/clang/3.4/include
	(cd ../../lib/clang/include && ${MAKE} includes)
	(cd ../../lib/clang/libllvmtablegen && ${MAKE} depend )
	(cd ../../lib/clang/libllvmtablegen && ${MAKE} all)
	(cd ../../lib/clang/libllvmsupport && ${MAKE} depend)
	(cd ../../lib/clang/libllvmsupport && ${MAKE} all)
	(cd ../../usr.bin/clang/clang-tblgen && ${MAKE} depend)
	(cd ../../usr.bin/clang/clang-tblgen && ${MAKE} all)
.if ${MACHINE_ARCH} == ${HOSTARCH}
	(cd ../../usr.bin/clang/clang-tblgen && ${SUDO} ${MAKE} install)
.endif
	(cd ../../usr.bin/clang/tblgen && ${MAKE} depend)
	(cd ../../usr.bin/clang/tblgen && ${MAKE} all)
.if ${MACHINE_ARCH} == ${HOSTARCH}
	(cd ../../usr.bin/clang/tblgen && ${SUDO} ${MAKE} install)
.endif
	(cd ../../lib/clang && ${MAKE} depend)
	(cd ../../lib/clang && ${MAKE} all)
	(cd ../../lib/clang && ${MAKE} depend)
	(cd ../../lib/clang && ${MAKE} all)
	(cd ../../usr.bin/clang && ${MAKE} depend)
	(cd ../../usr.bin/clang && ${MAKE} all)

snapstrap:
	mkdir -p /usr/lib/clang/3.4/include
	(cd ../../lib/clang/libllvmtablegen && ${MAKE} depend )
	(cd ../../lib/clang/libllvmtablegen && ${MAKE} all)
	(cd ../../lib/clang/libllvmsupport && ${MAKE} depend)
	(cd ../../lib/clang/libllvmsupport && ${MAKE} all)
	(cd ../../usr.bin/clang/clang-tblgen && ${MAKE} depend)
	(cd ../../usr.bin/clang/clang-tblgen && ${MAKE} all)
	(cd ../../usr.bin/clang/clang-tblgen && ${SUDO} ${MAKE} install)
	(cd ../../usr.bin/clang/tblgen && ${MAKE} depend)
	(cd ../../usr.bin/clang/tblgen && ${MAKE} all)
	(cd ../../usr.bin/clang/tblgen && ${SUDO} ${MAKE} install)
	(cd ../../lib/clang/include && ${MAKE} includes)
	
snapclean:
	(cd ../../lib/clang/include && ${MAKE} cleandir)
	(cd ../../lib/clang/libllvmtablegen && ${MAKE} cleandir)
	(cd ../../lib/clang/libllvmsupport && ${MAKE} cleandir)
	(cd ../../usr.bin/clang/clang-tblgen && ${MAKE} cleandir)
	(cd ../../usr.bin/clang/tblgen && ${MAKE} cleandir)

.include <bsd.subdir.mk>
