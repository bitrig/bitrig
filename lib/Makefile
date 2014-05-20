#	$OpenBSD: Makefile,v 1.67 2014/04/22 10:21:56 reyk Exp $
#	$NetBSD: Makefile,v 1.20.4.1 1996/06/14 17:22:38 cgd Exp $

.include <bsd.own.mk>

SUBDIR=	csu libarch libc libc++ libc++abi libcrypto libcurses libedit \
	libevent libexpat libform libfuse libkeynote libkvm libl libm libmenu \
	libocurses libossaudio libpanel libpcap librthread librpcsvc \
	libskey libsndio libsqlite3 libssl libusbhid libutil liby libz
.if ${MK_ENABLE_CLANG:L} == "yes"
SUBDIR+=libcompiler_rt
SUBDIR+=clang
.endif

.include <bsd.subdir.mk>
