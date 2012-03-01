#	$OpenBSD: Makefile,v 1.51 2011/11/21 01:36:18 jsg Exp $
#	$NetBSD: Makefile,v 1.35 1996/04/03 21:20:52 chuck Exp $

.include <bsd.own.mk>

SUBDIR+=	cc cxxfilt lynx

.if make(obj)
SUBDIR+=	binutils
.else
SUBDIR+=	${BINUTILS_VERSION}
.endif

# Do these last; texinfo builds the info 'dir' file, perl must be after binutils
SUBDIR+=	perl

.include <bsd.subdir.mk>
