#	$OpenBSD: Makefile,v 1.69 2014/10/31 13:46:17 jsing Exp $
#	$NetBSD: Makefile,v 1.20.4.1 1996/06/14 17:22:38 cgd Exp $

.include <bsd.own.mk>

SUBDIR=	csu libarch libc libc++ libc++abi libcrypto libcurses libedit \
	libevent libexpat libform libfuse libkeynote libkvm libl libm libmenu \
	libocurses libossaudio libpanel libpcap librthread librpcsvc \
	libskey libsndio libsqlite3 libssl libtls libusbhid libutil liby libz
.if ${MK_ENABLE_CLANG:L} == "yes"
SUBDIR+=libcompiler_rt
SUBDIR+=clang
.endif

.include <bsd.subdir.mk>
