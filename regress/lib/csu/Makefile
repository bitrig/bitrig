#	$OpenBSD: Makefile,v 1.5 2014/11/23 08:46:49 guenther Exp $

SUBDIR+= ctors dtors init_priority order_test

.if !defined(NOPIC)
SUBDIR+= callbacks
.endif

install:

.include <bsd.subdir.mk>
