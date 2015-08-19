#	$OpenBSD: Makefile,v 1.1 2014/12/23 17:16:02 deraadt Exp $

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_subr.c ffs_tables.c \
	vfs_wapbl.c wapbl.c
CFLAGS+= -I${.CURDIR}/../../../sbin/fsck
CPPFLAGS+=-DWAPBL_DEBUG_PRINT=0
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.PATH:  ${.CURDIR}/../../../sbin/fsck_ffs
.PATH:	${.CURDIR}/../../../sys/ufs/ffs 
.PATH:	${.CURDIR}/../../../sys/kern
.PATH:	${.CURDIR}/../../../sbin/fsck
.include <bsd.prog.mk>
