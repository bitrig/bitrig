#	$OpenBSD: Makefile,v 1.41 2014/03/18 22:36:31 miod Exp $
#	$NetBSD: Makefile,v 1.5 1995/09/15 21:05:21 pk Exp $

SUBDIR=	dev/microcode \
	arch/amd64 arch/armv7 \
	arch/i386

tags:
	cd ${.CURDIR}/kern; make tags

.include <bsd.subdir.mk>
