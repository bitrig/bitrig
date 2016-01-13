
REV=	${OSrev}

IMAGE=	miniroot-${BOARD}-${REV}.fs
MKUBOOT?=	mkuboot

MOUNT_POINT=	/mnt

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_CDEV=	/dev/${VND}c
VND_IDEV=	/dev/${VND}i
VND_JDEV=	/dev/${VND}j
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
VND_RIDEV=	/dev/r${VND}i
VND_RJDEV=	/dev/r${VND}j
VND_FSDEV?=	${VND_IDEV}
VND_RFSDEV?=	${VND_RIDEV}
PID!=		echo $$$$

NBLKS=		36864

FS?=		msdos
PART_ID?=C

NEWFS_ARGS_msdos=-F 16 -L boot
NEWFS_ARGS_ext2fs=-v boot

cleandir: clean
clean:
	rm -f ${IMAGE}

.ifndef DESTDIR
all ${IMAGE}:
	@echo setenv DESTDIR before making a ramdisk!
	@false
.else
all:	${IMAGE}

.ORDER: rd_setup copy_md_files_pre do_files copy_md_files rd_teardown
${IMAGE}: rd_setup copy_md_files_pre do_files copy_md_files rd_teardown

.endif

copy_md_files:
copy_md_files_pre:

do_files:
	cp ${.OBJDIR}/../../ramdisk/bsd.rd.${PLATFORM}.umg ${MOUNT_POINT}/bsd.umg

.ORDER: rd_setup_pre rd_setup_md rd_setup_post
rd_setup: rd_setup_pre rd_setup_md rd_setup_post
rd_setup_pre:
	dd if=/dev/zero of=${IMAGE} bs=512 count=${NBLKS}
	vnconfig -c ${VND} ${IMAGE}

rd_setup_md:

rd_setup_post:
	newfs_${FS} ${NEWFS_ARGS_${FS}} ${VND_RFSDEV} >/dev/null
	mount ${VND_FSDEV} ${MOUNT_POINT}

rd_teardown:
	@df -i ${MOUNT_POINT}
	-umount ${MOUNT_POINT}
	-vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

.PRECIOUS:	${IMAGE}

.ifdef RELEASEDIR
install:
	cp ${IMAGE} ${RELEASEDIR}
.endif

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.own.mk>
