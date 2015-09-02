#	$OpenBSD: Makefile.inc,v 1.16 2015/09/02 07:49:42 deraadt Exp $

KERNELS += GENERIC.SP bsd.sp

bootblocks:
	cp ${DESTDIR}/usr/mdec/pxeboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdbr ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/BOOTX64.EFI ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/BOOTX86.EFI ${RELEASEDIR}

MDEXT=	bsd.rd cd${OSrev}.iso usb${OSrev}.fs pxeboot cdboot cdbr \
	BOOTX64.EFI BOOTX86.EFI
