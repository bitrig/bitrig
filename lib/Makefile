#	$OpenBSD: Makefile,v 1.71 2015/11/14 23:56:49 deraadt Exp $
#	$NetBSD: Makefile,v 1.20.4.1 1996/06/14 17:22:38 cgd Exp $

.include <bsd.own.mk>

SUBDIR=	csu libarch libc libc++ libc++abi libcrypto libcurses libedit \
	libevent libexpat libform libfuse libkeynote libkvm libl libm libmenu \
	libossaudio libpanel libpcap libradius librthread librpcsvc \
	libskey libsndio libsqlite3 libssl libtls libusbhid libutil liby libz
.if ${MK_ENABLE_CLANG:L} == "yes"
SUBDIR+=libcompiler_rt
SUBDIR+=clang
.endif

.include <bsd.subdir.mk>
