#	$OpenBSD: Makefile.inc,v 1.15 2015/09/02 04:13:15 deraadt Exp $

KERNELS += GENERIC.SP bsd.sp

bootblocks:
	cp ${DESTDIR}/usr/mdec/pxeboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdbr ${RELEASEDIR}

MDEXT=	bsd.rd cd${OSrev}.iso usb${OSrev}.fs pxeboot cdboot cdbr \
	BOOTX64.EFI BOOTX86.EFI
