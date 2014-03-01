.include <bsd.own.mk>
.include <sys.mk>

BSD =		LIVECD
BOOTIMG =	boot.img
BOOTIMGSIZE =	16M
VND ?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
SOURCE=		${DESTDIR}	
SETS=		${RELEASEDIR}
REV=		${OSrev}

bsd:
	cd ${.CURDIR}/../../sys/arch/${MACHINE_ARCH}/conf && config ${BSD}
	cd ${.CURDIR}/../../sys/arch/${MACHINE_ARCH}/compile/${BSD} && \
		    ${MAKE} clean && exec ${MAKE}
	cp ${.CURDIR}/../../sys/arch/${MACHINE_ARCH}/compile/${BSD}/bsd bsd

${BOOTIMG}: bsd
	dd if=/dev/zero of=${.OBJDIR}/${BOOTIMG} count=1 bs=${BOOTIMGSIZE}
	vnconfig -v -c ${VND} ${BOOTIMG}
	fdisk -yi ${VND}
	disklabel -wA ${VND} ${FLOPPYTYPE}
	newfs ${VND_RDEV}
	mkdir -p ${.OBJDIR}/mnt
	mount ${VND_DEV} ${.OBJDIR}/mnt
	cp ${.OBJDIR}/bsd ${.OBJDIR}/mnt/
	cp /usr/mdec/boot ${.OBJDIR}/mnt/
	/usr/mdec/installboot -v ${.OBJDIR}/mnt/boot /usr/mdec/biosboot ${VND}
	sync
	sleep 2
	umount ${.OBJDIR}/mnt
	vnconfig -u ${VND}

.content_done: ${BOOTIMG}
	mkdir -p ${.OBJDIR}/fs
	cp -Rp ${.OBJDIR}/${BOOTIMG} ${.OBJDIR}/fs/
	cd ${SOURCE}; tar cf - * .* | tar xpf - -C ${.OBJDIR}/fs/

#	delete all devel paraphernalia
	rm -rf ${.OBJDIR}/fs/usr/include
	rm -rf ${.OBJDIR}/fs/usr/lib/*.a ${.OBJDIR}/fs/usr/lib/*.la
	rm -rf ${.OBJDIR}/fs/usr/X11R6/include
	rm -rf ${.OBJDIR}/fs/usr/X11R6/lib/*.a ${.OBJDIR}/fs/usr/X11R6/lib/*.la

#	strip all binaries and compress executables
	find ${.OBJDIR}/fs/ -type f -exec strip {} 2> /dev/null \;
#	XXX drahn fix the gzexe stuff over here

#	patch up release for use
	echo "/dev/cd0a / cd9660 ro 0 0" > ${.OBJDIR}/fs/etc/fstab
	echo "machdep.allowaperture=2" >> ${.OBJDIR}/fs/etc/sysctl.conf
	cd  ${.OBJDIR}/fs/etc && patch < ${.CURDIR}/mfs.ptch
	cd ${.OBJDIR}/fs/dev && sh MAKEDEV all

#	save mfs directories
	mkdir -p ${.OBJDIR}/fs/ro/
	cd ${.OBJDIR}/fs; tar zcf ${.OBJDIR}/fs/ro/mfs.tgz dev etc var root

#	copy sets
	mkdir -p ${.OBJDIR}/fs/sets
	cp ${SETS}/bsd* ${.OBJDIR}/fs/sets
	cp ${SETS}/*tgz ${.OBJDIR}/fs/sets
	ls -la ${.OBJDIR}/fs/sets/ > ${.OBJDIR}/fs/sets/index.txt

#	copy install scripts
	cp ${.CURDIR}/../miniroot/upgrade.sh ${.OBJDIR}/fs/upgrade
	cp ${.CURDIR}/../miniroot/install.sh ${.OBJDIR}/fs/install
	cp ${.CURDIR}/../miniroot/install.sub ${.OBJDIR}/fs/install.sub
	cp ${.CURDIR}/../`uname -m`/common/install.md ${.OBJDIR}/fs/install.md
	mkdir -p ${.OBJDIR}/fs/mnt2
	chmod 755 ${.OBJDIR}/fs/upgrade
	chmod 755 ${.OBJDIR}/fs/install

#	and we're done
	touch ${.OBJDIR}/.content_done

livecd${REV}.iso: .content_done
	mkisofs  -R -T -l -d -D -N -v -v -c boot.catalog -hard-disk-boot \
		-o ${.OBJDIR}/livecd${REV}.iso \
		-G ${.OBJDIR}/${BOOTIMG} \
		-eltorito-boot ${BOOTIMG} \
		${.OBJDIR}/fs

clean cleandir:
	rm -rf ${.OBJDIR}/${BOOTIMG} \
		${.OBJDIR}/livecd${REV}.iso \
		${.OBJDIR}/mnt \
		${.OBJDIR}/.content_done \
		${.OBJDIR}/fs \
		${.OBJDIR}/bsd


.if !defined(DESTDIR) || !defined(RELEASEDIR)
all: livecd${REV}.iso
	@echo setenv DESTDIR before doing that!
	@false
.else
all: livecd${REV}.iso
.endif

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
