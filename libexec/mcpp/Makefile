CFLAGS+=-I. -I${.CURDIR} -I${.SRCDIR}/dist-mcpp/src -DHAVE_CONFIG_H -DCPU='"${MACHINE_ARCH}"'

PROG=mcpp

.PATH: ${.CURDIR}/../../contrib/mcpp/src

SRCS= directive.c main.c system.c eval.c mbchar.c
SRCS+= expand.c support.c

ARCHS=amd64 arm

MLINKS+= mcpp.1 cpp.1

# generate #define file mcpp expects during runtime
DOTH_MAJOR!=${CC} -E -dM - </dev/null | grep __GNUC__ | awk '{ print $$3 }'
DOTH_MINOR!=${CC} -E -dM - </dev/null | grep __GNUC_MINOR__ | awk '{ print $$3 }'
DOTH_FILE!=printf "gcc%d%d_predef_old.h" ${DOTH_MAJOR} ${DOTH_MINOR}
DOTH_FILE2!=printf "gcc%d%d_predef_std.h" ${DOTH_MAJOR} ${DOTH_MINOR}
DOTH_FILE3!=printf "gxx%d%d_predef_std.h" ${DOTH_MAJOR} ${DOTH_MINOR}
ARCHPREFIX=mcpp-gcc-

includes:
.for arch in ${ARCHS}
	${INSTALL} ${INSTALL_COPY} -m 444 ${.CURDIR}/defs.old.${arch} \
		${DESTDIR}/usr/include/${ARCHPREFIX}${arch}/${DOTH_FILE}
	${INSTALL} ${INSTALL_COPY} -m 444 ${.CURDIR}/defs.std.${arch} \
		${DESTDIR}/usr/include/${ARCHPREFIX}${arch}/${DOTH_FILE2}
	ln -f ${DESTDIR}/usr/include/${ARCHPREFIX}${arch}/${DOTH_FILE2} \
		${DESTDIR}/usr/include/${ARCHPREFIX}${arch}/${DOTH_FILE3}
.endfor

.include <bsd.prog.mk>

