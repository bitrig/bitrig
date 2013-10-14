#	$OpenBSD: Makefile,v 1.42 2013/10/14 00:00:12 guenther Exp $

REV=		${OSrev}

CBIN=		instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/ramdisk

.include <bsd.own.mk>

.if exists(${ARCHDIR}/Makefile.inc)
.include "${ARCHDIR}/Makefile.inc"
.endif

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
DISKTYPE=	rdroot
PID!=		echo $$$$
REALIMAGE!=	echo /var/tmp/image.${PID}
IMAGE?=		ramdisk${REV}.fs
IMAGESIZE?=	8192 # 4MB in 512 byte blocks
NEWFS_WILL_FAIL?=false
RDSETROOT?=	elfrdsetroot
RDSETROOT_EXT_SRC=${.CURDIR}/../common/elf32.c ${.CURDIR}/../common/elf64.c
CRUNCHGENOPTS?=-E
KERNEL?=	RAMDISK
UTILS?=		${.CURDIR}/../miniroot
STRIP?=		strip

LISTS=		${.CURDIR}/list ${ARCHDIR}/list.local
MTREE=		${UTILS}/mtree.conf

INSTALLBOOT?=	installboot
BOOT?=		${DESTDIR}/usr/mdec/boot
BOOTIN?=	${MOUNT_POINT}/boot
BOOTXX?=	${DESTDIR}/usr/mdec/bootxx

# old format, minfree, opt, b/i  trks, sects, cpg
NEWFSOPTS=	-t ffs -m 0 -o space -f 1024 -b 8192
NEWFSOPTS_RD?=	${NEWFSOPTS} -i 8192 -s ${IMAGESIZE}

.ifndef DESTDIR
all:
	@echo setenv DESTDIR before making a ramdisk!
	@false
.else

all:	bsd.rd

bsd:
	cd ${.CURDIR}/../../sys/arch/${MACHINE}/conf && config ${KERNEL}
	cd ${.CURDIR}/../../sys/arch/${MACHINE}/compile/${KERNEL} && \
	    ${MAKE} clean && ${MAKE} depend && COPTS=-Os ${MAKE}
	cp ${.CURDIR}/../../sys/arch/${MACHINE}/compile/${KERNEL}/bsd bsd

bsd.rd:	bsd ${IMAGE} ${RDSETROOT}
	cp bsd bsd.rd
	${.OBJDIR}/${RDSETROOT} bsd.rd ${IMAGE}
	${STRIP} bsd.rd

bsd.gz:	bsd.rd
	${GZIPCMD} ${GZIPFLAGS} < bsd.rd > bsd.gz

${RDSETROOT}: ${.CURDIR}/../common/${RDSETROOT}.c ${RDSETROOT_EXT_SRC}
	${HOSTCC} ${HOSTCCFLAGS} -o ${RDSETROOT} ${RDSETROOT_EXT_SRC} \
	    ${.CURDIR}/../common/${RDSETROOT}.c

${IMAGE}: ${CBIN}
	dd if=/dev/zero of=${REALIMAGE} count=${IMAGESIZE}
	vnconfig -v -c ${VND} ${REALIMAGE}
	disklabel -w ${VND} ${DISKTYPE}
	newfs ${NEWFSOPTS_RD} ${VND_RDEV} || ${NEWFS_WILL_FAIL}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	REV=${REV} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    ARCHDIR=${ARCHDIR} TARGDIR=${MOUNT_POINT} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${MOUNT_POINT}/${CBIN}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	cp ${REALIMAGE} ${IMAGE}
	rm ${REALIMAGE}

${CBIN}.conf: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CBIN}.conf
	crunchgen ${CRUNCHGENOPTS} -D ${.CURDIR}/../.. -L ${DESTDIR}/usr/lib \
	    ${CBIN}.conf

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk SRCLIBDIR=${.CURDIR}/../../../lib all

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	    lib*.a lib*.olist ${CBIN}.map \
	    ${CBIN}.conf *.o *.lo *.c bsd bsd.*

.endif	# DESTDIR check

.ifdef RELEASEDIR
beforeinstall:
	cp bsd.rd ${RELEASEDIR}
.endif	# RELEASEDIR

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${REALIMAGE}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
