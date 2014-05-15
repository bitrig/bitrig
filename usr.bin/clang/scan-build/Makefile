.include <bsd.own.mk>

MAN=scan-build.1
NOPROG=

VPATH=${.CURDIR}/../../../contrib/llvm/tools/clang/tools/scan-build
SRCDIR=${.CURDIR}/../../../contrib/llvm/tools/clang/tools/scan-build
BINDIR=/usr/bin

DATDIR=/usr/share/llvm
DATFILE=scanview.css sorttable.js
PERLBIN=c++-analyzer ccc-analyzer scan-build
PYBIN=set-xcode-analyzer

.for i in ${PYBIN}
CLEANFILES+=${i}.fix
all:  ${.OBJDIR}/${i}.fix

${i}.fix: ${i}
	sed -e 's,#!/usr/bin/python,#!/usr/bin/env python,' < ${?} \
		> ${.OBJDIR}/${i}.fix
.endfor

.for i in ${PERLBIN}
CLEANFILES+=${i}.fix
all:  ${.OBJDIR}/${i}.fix

${i}.fix: ${i}
	sed -e "s,\"\$$RealBin/sorttable,\"${DATDIR}/sorttable," \
		-e "s,\"\$$RealBin/scanview,\"${DATDIR}/scanview," \
		< ${?} > ${.OBJDIR}/${i}.fix
.endfor

realinstall:
.for i in ${PERLBIN}
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/${i}.fix ${DESTDIR}${BINDIR}/$i
.endfor
.for i in ${PYBIN}
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.OBJDIR}/${i}.fix ${DESTDIR}${BINDIR}/$i
.endfor
.for i in ${DATFILE}
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${SRCDIR}/${i} ${DESTDIR}${DATDIR}/$i
.endfor

.include "bsd.prog.mk"
