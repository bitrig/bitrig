#	$OpenBSD: Makefile,v 1.17 2014/11/10 19:06:47 guenther Exp $

.if make(obj)
SUBDIR= amd64 arm
.else
SUBDIR= ${MACHINE_CPU}
.endif

.include <bsd.subdir.mk>
