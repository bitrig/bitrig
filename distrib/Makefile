#	$OpenBSD: Makefile,v 1.39 2011/07/07 19:16:43 deraadt Exp $

SUBDIR=	special notes

.if	make(obj)
SUBDIR+=amd64 armish beagle i386 zaurus
.elif exists(${MACHINE})
SUBDIR+=${MACHINE}
.endif

.include <bsd.subdir.mk>
