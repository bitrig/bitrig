CFLAGS+=-I. -I${.CURDIR} -I${.SRCDIR}/dist-mcpp/src -DHAVE_CONFIG_H -DCPU='".${MACHINE_ARCH}"'

PROG=mcpp

.PATH: ${.CURDIR}/../../contrib/mcpp/src

SRCS= directive.c main.c system.c eval.c mbchar.c
SRCS+= expand.c support.c

# generate #define file mcpp expects during runtime
DOTH_MAJOR!=${CC} -E -dM - </dev/null | grep __GNUC__ | awk '{ print $$3 }'
DOTH_MINOR!=${CC} -E -dM - </dev/null | grep __GNUC_MINOR__ | awk '{ print $$3 }'
DOTH_FILE=`printf "gcc%d%d_predef_old.h" ${DOTH_MAJOR} ${DOTH_MINOR}`
DOTH_FILE2=`printf "gcc%d%d_predef_std.h" ${DOTH_MAJOR} ${DOTH_MINOR}`
DOTH_FILE3=`printf "gxx%d%d_predef_std.h" ${DOTH_MAJOR} ${DOTH_MINOR}`
ARCHDIR=mcpp-gcc-.${MACHINE_ARCH}

includes:
	${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/${ARCHDIR}
	${CC} -E -dM - </dev/null > ${DOTH_FILE}
	${INSTALL} ${INSTALL_COPY} -m 444 ${DOTH_FILE} \
		${DESTDIR}/usr/include/${ARCHDIR}
	ln -f ${DESTDIR}/usr/include/${ARCHDIR}/${DOTH_FILE} \
		${DESTDIR}/usr/include/${ARCHDIR}/${DOTH_FILE2}
	ln -f ${DESTDIR}/usr/include/${ARCHDIR}/${DOTH_FILE} \
		${DESTDIR}/usr/include/${ARCHDIR}/${DOTH_FILE3}

.include <bsd.prog.mk>

