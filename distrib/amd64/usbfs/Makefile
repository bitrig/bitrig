TOP=		${.CURDIR}/..
REV=		${OSrev}
MOUNT_POINT=	/mnt

FS?=		usb${REV}.fs

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
PID!=		echo $$$$
REALIMAGE!=	echo /var/tmp/image.${PID}
BIOSBOOT?=	${DESTDIR}/usr/mdec/biosboot
BOOT?=		${DESTDIR}/usr/mdec/boot
MBR?=		${DESTDIR}/usr/mdec/mbr

IMAGESIZE=	16640

.NOTPARALLEL:

all:	${FS}

${FS}:	bsd.gz ${BIOSBOOT} ${BOOT} ${MBR}
	dd if=/dev/zero of=${REALIMAGE} bs=512 count=${IMAGESIZE}
	vnconfig -v -c ${VND} ${REALIMAGE}
	fdisk -yi -l ${IMAGESIZE} -f ${MBR} ${VND}
	disklabel -w -A ${VND}
	newfs -m 0 -o space -i 524288 -c ${IMAGESIZE} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${BOOT} ${.OBJDIR}/boot
	strip ${.OBJDIR}/boot
	strip -R .comment ${.OBJDIR}/boot
	installboot -v -r ${MOUNT_POINT} ${VND_CRDEV} \
	    ${BIOSBOOT} ${.OBJDIR}/boot
	dd if=bsd.gz of=${MOUNT_POINT}/bsd bs=512
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}
	cp ${REALIMAGE} ${FS}
	rm ${REALIMAGE}

bsd.gz: ${.OBJDIR}/../ramdisk/bsd.rd
	gzip -c9n ${.OBJDIR}/../ramdisk/bsd.rd > bsd.gz

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

.ifdef RELEASEDIR
install:
	cp ${FS} ${RELEASEDIR}/${FS}
.endif	# RELEASEDIR

clean cleandir:
	/bin/rm -f bsd.gz boot ${FS}

.include <bsd.own.mk>
.include <bsd.obj.mk>
