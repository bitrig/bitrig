RAMDISK=	RAMDISK
TOP=		${.CURDIR}/..
REV=		${OSrev}
IMAGE=		mr.fs
CBIN?=		instbin
CRUNCHCONF?=	${CBIN}.conf
LISTS=		${.CURDIR}/list
UTILS?=		${.CURDIR}/../../miniroot

MOUNT_POINT=	/mnt
MTREE=		${UTILS}/mtree.conf
TMPFS_MAXSIZE=	255M

.if ${COMPILER_VERSION:L} == "gcc4"
COPTS=	-Os
.else
COPTS=	-Oz
.endif

.NOTPARALLEL:

all:	bsd.rd

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
