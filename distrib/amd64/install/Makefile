TOP=	${.CURDIR}/..
BSDRD=	bsd.rd
CDROM=	install${OSrev}.iso

.NOTPARALLEL:

.if !(defined(RELEASEDIR) && defined(XRELEASEDIR))
all:
	@echo "RELEASEDIR and XRELEASEDIR must point to a valid snap directories"
	@exit 1
.else
FILES=	BOOTIA32.EFI BOOTX64.EFI BUILDINFO SHA256 bsd bsd.sp base10.tgz
FILES+=	comp10.tgz game10.tgz pxeboot
SIG=	SHA256.sig
XFILES=	xbase10.tgz  xfont10.tgz  xserv10.tgz  xshare10.tgz
all: ${CDROM}

.endif

${CDROM}: ${.OBJDIR}/../ramdisk/${BSDRD} ${DESTDIR}/usr/mdec/cdbr ${DESTDIR}/usr/mdec/cdboot
	-rm -rf ${.OBJDIR}/cd-dir
	mkdir -p ${.OBJDIR}/cd-dir/${OSREV}/amd64
	mkdir -p ${.OBJDIR}/cd-dir/etc
	echo "set image /${OSREV}/amd64/bsd.rd" > ${.OBJDIR}/cd-dir/etc/boot.conf
	cp ${.OBJDIR}/../ramdisk/${BSDRD} ${.OBJDIR}/cd-dir/${OSREV}/amd64
	cp ${DESTDIR}/usr/mdec/cdbr ${.OBJDIR}/cd-dir/${OSREV}/amd64
	cp ${DESTDIR}/usr/mdec/cdboot ${.OBJDIR}/cd-dir/${OSREV}/amd64/cdboot
	for i in ${FILES}; do \
		cp ${RELEASEDIR}/$$i ${.OBJDIR}/cd-dir/${OSREV}/amd64/$$i; \
	done
	for i in ${XFILES}; do \
		cp ${XRELEASEDIR}/$$i ${.OBJDIR}/cd-dir/${OSREV}/amd64/$$i; \
	done
	cat ${XRELEASEDIR}/SHA256 >> ${.OBJDIR}/cd-dir/${OSREV}/amd64/SHA256
	cp ${RELEASEDIR}/${SIG} ${.OBJDIR}/cd-dir/${OSREV}/amd64/${SIG} || true
	ls ${.OBJDIR}/cd-dir/${OSREV}/amd64/ > ${.OBJDIR}/cd-dir/${OSREV}/amd64/index.txt
	mkisofs -R -T -allow-leading-dots -l -d -D -N -o ${.OBJDIR}/${CDROM} -v -v \
	    -A "Bitrig ${OSREV} amd64 install CD" \
	    -publisher "Copyright (c) `date +%Y`, The Bitrig Project <bitrig@bitrig.org>" \
	    -p "The Bitrig Project <bitrig@bitrig.org>" \
	    -V "Bitrig/amd64 ${OSREV} install CD" \
	    -no-emul-boot -b ${OSREV}/amd64/cdbr -c ${OSREV}/amd64/boot.catalog \
	    ${.OBJDIR}/cd-dir

clean cleandir:
	/bin/rm -f ${CDROM}
	rm -rf cd-dir

.include <bsd.own.mk>
.include <bsd.obj.mk>
