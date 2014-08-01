#	$OpenBSD: Makefile,v 1.7 2013/11/01 20:35:52 jasper Exp $

REV=	${OSrev}

TOP=		${.CURDIR}/..

BSD_RD=		bsd.rd
IMAGE=		mr.fs
CBIN?=		instbin
CRUNCHCONF?=	${CBIN}.conf
CRUNCHCONFS?=	${.CURDIR}/../common/crunch.conf
LISTS=		${.CURDIR}/list
UTILS?=		${.CURDIR}/../../miniroot

MOUNT_POINT=	/mnt
MTREE=		${UTILS}/mtree.conf

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
PID!=		echo $$$$

TMPFS_MAXSIZE=	64M

KERNADDR_OMAP=0x80300000
KERNADDR_IMX=0x10300000
KERNADDR_EXYNOS=0x40800000
KERNADDR_SUNXI=0x40800000

.ifndef DESTDIR
all ${IMAGE}:
	@echo setenv DESTDIR before making a ramdisk!
	@false
.else

ALLSOC=		IMX OMAP EXYNOS SUNXI
ALLBSD=		bsd
ALLBSDRD=	${BSD_RD}
.for SOC in ${ALLSOC}
ALLBSDRD+=	${BSD_RD}.${SOC}.umg
all:	${ALLBSDRD}
.endfor

bsd:
	cd ${TOP}/../../sys/arch/armv7/conf && config RAMDISK
	cd ${TOP}/../../sys/arch/armv7/compile/RAMDISK && \
	    ${MAKE} clean && exec ${MAKE}
	cp ${TOP}/../../sys/arch/armv7/compile/RAMDISK/bsd bsd

${BSD_RD}: ${CBIN} ${IMAGE} bsd
	cp bsd ${BSD_RD}
	rdconfig bsd.rd ${IMAGE}
	rdconfig -I bsd.rd

.for SOC in ${ALLSOC}
${BSD_RD}.${SOC}.umg:
	mkuboot -a arm -o linux -e ${KERNADDR_${SOC}} -l ${KERNADDR_${SOC}} \
	    ${BSD_RD} ${BSD_RD}.${SOC}.umg
.endfor

${IMAGE}: tmpfs_setup do_files tmpfs_snapshot tmpfs_teardown

.endif

tmpfs_setup:
	mount -t tmpfs -o -s${TMPFS_MAXSIZE} tmpfs ${MOUNT_POINT}

tmpfs_snapshot:
	rm -f ${IMAGE}
	mount -t tmpfs -u -o snapshot ${IMAGE} ${MOUNT_POINT}

tmpfs_teardown:
	@df -i ${MOUNT_POINT}
	-umount ${MOUNT_POINT}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

.PRECIOUS:	${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -E -D ${.CURDIR}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

${CRUNCHCONF}: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

do_files:
	cat ${MTREE} | mtree -de -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    REV=${REV} TARGDIR=${MOUNT_POINT} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${MOUNT_POINT}/${CBIN}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	    lib*.a lib*.olist ${CBIN}.map \
	    *.o *.lo *.c ${ALLBSD} ${ALLBSDRD}
	rm -f instbin.conf
	rm -rf cd-dir

.ifdef RELEASEDIR
install:
	cp ${ALLBSDRD} ${RELEASEDIR}
.endif	# RELEASEDIR

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
