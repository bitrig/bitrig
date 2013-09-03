#	$OpenBSD: Makefile,v 1.1 2013/05/30 19:17:15 bmercer Exp $

BINDIR=	/usr/sbin
PROG=	mkuboot
SRCS=	mkuboot.c copy_elf32.c copy_elf64.c
DPADD=	${LIBZ}
LDADD=	-lz
CFLAGS=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

MAN=	mkuboot.8

.include <bsd.prog.mk>
