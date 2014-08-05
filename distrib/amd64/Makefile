#	$OpenBSD: Makefile,v 1.4 2007/10/18 18:16:20 deraadt Exp $

SUBDIR= ramdisk cdfs usbfs

unconfig:
	cd ramdisk; ${MAKE} unconfig
	cd usbfs; ${MAKE} unconfig

.include <bsd.subdir.mk>
