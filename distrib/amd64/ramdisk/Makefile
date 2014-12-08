TOP=		${.CURDIR}/..
ARCH=		${.CURDIR}/../../../sys/arch/amd64
REV=		${OSrev}
IMAGE=		mr.fs
CBIN?=		instbin
CRUNCHCONF?=	${CBIN}.conf
LISTS=		${.CURDIR}/list
UTILS?=		${.CURDIR}/../../miniroot

FSDIR=		${.OBJDIR}/fs
MTREE=		${UTILS}/mtree.conf

.NOTPARALLEL:

all:	bsd.rd

bsd.rd:	${IMAGE} bsd
	cp bsd bsd.rd
	rdconfig bsd.rd ${IMAGE}
	rdconfig -I bsd.rd

bsd:
	stat ${ARCH}/compile/GENERIC/bsd || \
		{ cd ${ARCH}/conf && config GENERIC && \
		  cd ${ARCH}/compile/GENERIC && ${MAKE} clean && \
		  exec ${MAKE}; }
	cp ${ARCH}/compile/GENERIC/bsd bsd

${IMAGE}: ${CBIN}
	rm -rf ${FSDIR}
	install -d -o root -g wheel ${FSDIR}
	mtree -def ${MTREE} -p ${FSDIR}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    REV=${REV} TARGDIR=${FSDIR} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${FSDIR}/${CBIN}
	(cd ${FSDIR} && pax -w -f ${.OBJDIR}/${IMAGE} -x tmpfs {,.}*)

.PRECIOUS:	${IMAGE}

.ifdef RELEASEDIR
install:
	cp bsd.rd ${RELEASEDIR}/bsd.rd
.endif	# RELEASEDIR

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -E -D ${BSDSRCDIR} -L ${DESTDIR}/usr/lib \
	-c ${CBIN}.c -e ${CBIN} -m ${CBIN}.mk ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all
	strip -R .comment ${CBIN}

${CRUNCHCONF}: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CRUNCHCONF}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}*.cache \
	    *.o *.lo *.c bsd bsd.rd bsd.gz bsd.strip \
	    lib*.a lib*.olist ${CBIN}.map boot ${CRUNCHCONF}
	/bin/rm -rf ${FSDIR}

.include <bsd.own.mk>
.include <bsd.obj.mk>
