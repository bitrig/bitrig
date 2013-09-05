#	$OpenBSD: Makefile,v 1.39 2011/07/07 19:16:43 deraadt Exp $

SUBDIR=	special

.if	make(obj)
SUBDIR+=amd64 armv7 i386
.elif exists(${MACHINE})
SUBDIR+=${MACHINE}
.endif

.include <bsd.subdir.mk>
