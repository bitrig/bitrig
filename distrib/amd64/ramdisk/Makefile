RAMDISK=	RAMDISK
FLOPPYSIZE=	11520
FLOPPYTYPE=	floppy576
XNAME=		cdrom
NOFS=		1
TOP=		${.CURDIR}/..
REV=		${OSrev}
IMAGE=		mr.fs
CBIN?=		instbin
CRUNCHCONF?=	${CBIN}.conf
LISTS=		${.CURDIR}/list
UTILS?=		${.CURDIR}/../../miniroot

MOUNT_POINT=	/mnt
MTREE=		${UTILS}/mtree.conf

FS?=		${XNAME}${REV}.fs
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
PID!=		echo $$$$
REALIMAGE!=	echo /var/tmp/image.${PID}
BOOT?=		${DESTDIR}/usr/mdec/boot

TMPFS_MAXSIZE=	255M

.if ${COMPILER_VERSION:L} == "gcc4"
COPTS=	-Os
.else
COPTS=	-Oz
.endif

.PATH: ${.CURDIR}/../ramdisk
.NOTPARALLEL:

all:	${FS}

${FS}:	bsd.gz
	dd if=/dev/zero of=${REALIMAGE} bs=512 count=${FLOPPYSIZE}
	vnconfig -v -c ${VND} ${REALIMAGE}
	disklabel -w ${VND} ${FLOPPYTYPE}
	newfs -m 0 -o space -i 524288 -c ${FLOPPYSIZE} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${BOOT} ${.OBJDIR}/boot
	strip ${.OBJDIR}/boot
	strip -R .comment ${.OBJDIR}/boot
	installboot -v -r ${MOUNT_POINT} ${VND_CRDEV} \
	    ${DESTDIR}/usr/mdec/biosboot ${.OBJDIR}/boot
	dd if=bsd.gz of=${MOUNT_POINT}/bsd bs=512
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}
	cp ${REALIMAGE} ${FS}
	rm ${REALIMAGE}

bsd.gz: bsd.rd
	cp bsd.rd bsd.strip
	strip bsd.strip
	strip -R .comment bsd.strip
	gzip -c9n bsd.strip > bsd.gz

bsd.rd:	${IMAGE} bsd
	cp bsd bsd.rd
	rdconfig bsd.rd ${IMAGE}
	rdconfig -I bsd.rd

bsd:
	cd ${.CURDIR}/../../../sys/arch/amd64/conf && config ${RAMDISK}
	cd ${.CURDIR}/../../../sys/arch/amd64/compile/${RAMDISK} && \
	    ${MAKE} clean && COPTS="${COPTS}" exec ${MAKE}
	cp ${.CURDIR}/../../../sys/arch/amd64/compile/${RAMDISK}/bsd bsd

${IMAGE}: ${CBIN} tmpfs_setup do_files tmpfs_snapshot tmpfs_teardown

tmpfs_setup: ${CBIN}
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

.ifdef RELEASEDIR
install:
.ifndef NOBSDRD
	cp bsd.rd ${RELEASEDIR}/bsd.rd
.endif
.ifndef NOFS
	cp ${FS} ${RELEASEDIR}/${FS}
.endif
.endif	# RELEASEDIR

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -E -D ${BSDSRCDIR} -L ${DESTDIR}/usr/lib \
	-c ${CBIN}.c -e ${CBIN} -m ${CBIN}.mk ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all
	strip -R .comment ${CBIN}

${CRUNCHCONF}: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CRUNCHCONF}

do_files:
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    REV=${REV} TARGDIR=${MOUNT_POINT} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${MOUNT_POINT}/${CBIN}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}*.cache \
	    *.o *.lo *.c bsd bsd.rd bsd.gz bsd.strip floppy*.fs \
	    lib*.a lib*.olist ${CBIN}.map boot ${CRUNCHCONF} ${FS}

.include <bsd.own.mk>
.include <bsd.obj.mk>
