#	$OpenBSD: Makefile,v 1.45 2014/03/18 22:36:27 miod Exp $

SUBDIR=	special

.if	make(obj)
SUBDIR+=amd64 armv7
.elif exists(${MACHINE})
SUBDIR+=${MACHINE}
.endif

.include <bsd.subdir.mk>
