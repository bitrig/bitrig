TOP=	${.CURDIR}/..
BSDRD=	bsd.rd
CDROM=	cd${OSrev}.iso

.NOTPARALLEL:

all: ${CDROM}

${CDROM}: ${.OBJDIR}/../ramdisk/${BSDRD} ${DESTDIR}/usr/mdec/cdbr ${DESTDIR}/usr/mdec/cdboot
	-rm -rf ${.OBJDIR}/cd-dir
	mkdir -p ${.OBJDIR}/cd-dir/${OSREV}/amd64
	mkdir -p ${.OBJDIR}/cd-dir/etc
	echo "set image /${OSREV}/amd64/bsd.rd" > ${.OBJDIR}/cd-dir/etc/boot.conf
	cp ${.OBJDIR}/../ramdisk/${BSDRD} ${.OBJDIR}/cd-dir/${OSREV}/amd64
	cp ${DESTDIR}/usr/mdec/cdbr ${.OBJDIR}/cd-dir/${OSREV}/amd64
	cp ${DESTDIR}/usr/mdec/cdboot ${.OBJDIR}/cd-dir/${OSREV}/amd64/cdboot
	mkisofs -R -T -allow-leading-dots -l -d -D -N -o ${.OBJDIR}/${CDROM} -v -v \
	    -A "Bitrig ${OSREV} amd64 bootonly CD" \
	    -publisher "Copyright (c) `date +%Y`, The Bitrig Project <bitrig@bitrig.org>" \
	    -p "The Bitrig Project <bitrig@bitrig.org>" \
	    -V "Bitrig/amd64 ${OSREV} boot-only CD" \
	    -no-emul-boot -b ${OSREV}/amd64/cdbr -c ${OSREV}/amd64/boot.catalog \
	    ${.OBJDIR}/cd-dir

.ifdef RELEASEDIR
install:
	cp ${CDROM} ${RELEASEDIR}
.endif

clean cleandir:
	/bin/rm -f ${CDROM}
	rm -rf cd-dir

.include <bsd.own.mk>
.include <bsd.obj.mk>
