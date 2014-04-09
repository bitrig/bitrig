#	$OpenBSD: Makefile,v 1.16 2011/07/04 05:54:27 deraadt Exp $

.if make(obj)
SUBDIR= amd64 arm
.else
SUBDIR= ${MACHINE_CPU}
.endif

.include <bsd.subdir.mk>
