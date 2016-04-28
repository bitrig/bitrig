#	$OpenBSD: Makefile,v 1.7 2013/11/01 20:35:52 jasper Exp $

REV=	${OSrev}

TOP=		${.CURDIR}/..
ARCH=		${.CURDIR}/../../../sys/arch/arm64

BSD_RD=		bsd.rd
IMAGE=		mr.fs
CBIN?=		instbin
CRUNCHCONF?=	${CBIN}.conf
LISTS=		${.CURDIR}/list
UTILS?=		${.CURDIR}/../../miniroot

FSDIR=		${.OBJDIR}/fs
MTREE=		${UTILS}/mtree.conf

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
PID!=		echo $$$$

KERNADDR_QEMU=0x40800000
KERNADDR_HIMEM=0x8000800000

ALLSOC=		QEMU HIMEM
ALLBSD=		bsd
ALLBSDRD=	${BSD_RD}
.for SOC in ${ALLSOC}
ALLBSDRD+=	${BSD_RD}.${SOC}.umg
.endfor

.ifdef CROSSDIR
HOSTARCH!= uname -m
OBJARG=-O obj.${HOSTARCH}.arm64
.endif

.ifndef DESTDIR
all ${IMAGE}:
	@echo setenv DESTDIR before making a ramdisk!
	@false
.else

all:	${ALLBSDRD}

bsd:
	stat ${ARCH}/compile/GENERIC/bsd || \
		{ cd ${ARCH}/conf && config GENERIC && \
		  cd ${ARCH}/compile/GENERIC && ${MAKE} clean && \
		  exec ${MAKE}; }
	cp ${ARCH}/compile/GENERIC/bsd bsd

${BSD_RD}: ${CBIN} ${IMAGE} bsd
	cp bsd ${BSD_RD}
	rdconfig bsd.rd ${IMAGE}
	rdconfig -I bsd.rd

.for SOC in ${ALLSOC}
${BSD_RD}.${SOC}.umg: ${BSD_RD}
	mkuboot -a arm -o linux -e ${KERNADDR_${SOC}} -l ${KERNADDR_${SOC}} \
	    ${BSD_RD} ${BSD_RD}.${SOC}.umg
.endfor

${IMAGE}:
	rm -rf ${FSDIR}
	install -d -o root -g wheel ${FSDIR}
	mtree -def ${MTREE} -p ${FSDIR}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    REV=${REV} TARGDIR=${FSDIR} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${FSDIR}/${CBIN}
	(cd ${FSDIR} && pax -w -f ${.OBJDIR}/${IMAGE} -x tmpfs {,.}*)

.endif

.PRECIOUS:	${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -E -D ${.CURDIR}/../../.. -L ${DESTDIR}/usr/lib ${OBJARG} ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

${CRUNCHCONF}: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	    lib*.a lib*.olist ${CBIN}.map \
	    *.o *.lo *.c ${ALLBSD} ${ALLBSDRD}
	rm -f instbin.conf
	rm -rf cd-dir
	/bin/rm -rf ${FSDIR}

.ifdef RELEASEDIR
install:
	cp ${ALLBSDRD} ${RELEASEDIR}
.endif	# RELEASEDIR

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
