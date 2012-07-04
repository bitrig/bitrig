TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BSDRD=	bsd.rd
CDROM=	cd${REV}.iso

all: ${CDROM}

${CDROM}:
	-rm -rf ${.OBJDIR}/cd-dir
	mkdir -p ${.OBJDIR}/cd-dir/${OSREV}/i386
	mkdir -p ${.OBJDIR}/cd-dir/etc
	echo "set image /${OSREV}/i386/bsd.rd" > ${.OBJDIR}/cd-dir/etc/boot.conf
	cp ${.OBJDIR}/../ramdisk/${BSDRD} ${.OBJDIR}/cd-dir/${OSREV}/i386
	cp ${DESTDIR}/usr/mdec/cdbr ${.OBJDIR}/cd-dir/${OSREV}/i386
	cp ${DESTDIR}/usr/mdec/cdboot ${.OBJDIR}/cd-dir/${OSREV}/i386/cdboot
	mkisofs -R -T -allow-leading-dots -l -d -D -N -o ${.OBJDIR}/${CDROM} -v -v \
	    -A "Bitrig ${OSREV} i386 bootonly CD" \
	    -publisher "Copyright (c) `date +%Y`, The Bitrig foundation <bitrig@bitrig.org>" \
	    -p "The Bitrig foundation <bitrig@bitrig.org>" \
	    -V "Bitrig/i386 ${OSREV} boot-only CD" \
	    -no-emul-boot -b ${OSREV}/i386/cdbr -c ${OSREV}/i386/boot.catalog \
	    ${.OBJDIR}/cd-dir

.ifdef RELEASEDIR
install:
	cp ${CDROM} ${RELEASEDIR}
.endif	# RELEASEDIR

clean cleandir:
	/bin/rm -f ${CDROM}
	rm -rf cd-dir

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
