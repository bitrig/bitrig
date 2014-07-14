#	$OpenBSD: Makefile,v 1.68 2014/07/14 00:49:03 jsing Exp $
#	$NetBSD: Makefile,v 1.20.4.1 1996/06/14 17:22:38 cgd Exp $

.include <bsd.own.mk>

SUBDIR=	csu libarch libc libc++ libc++abi libcrypto libcurses libedit \
	libevent libexpat libform libfuse libkeynote libkvm libl libm libmenu \
	libocurses libossaudio libpanel libpcap libressl librthread librpcsvc \
	libskey libsndio libsqlite3 libssl libusbhid libutil liby libz
.if ${MK_ENABLE_CLANG:L} == "yes"
SUBDIR+=libcompiler_rt
SUBDIR+=clang
.endif

.include <bsd.subdir.mk>
