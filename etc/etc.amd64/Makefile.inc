#	$OpenBSD: Makefile.inc,v 1.13 2011/07/22 09:30:54 espie Exp $

KERNELS += GENERIC.SP bsd.sp

bootblocks:
	cp ${DESTDIR}/usr/mdec/pxeboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdboot ${RELEASEDIR}
	cp ${DESTDIR}/usr/mdec/cdbr ${RELEASEDIR}

MDEXT=	bsd.rd cd${OSrev}.iso usb${OSrev}.fs pxeboot cdboot cdbr
