# $OpenBSD: Makefile,v 1.4 2011/04/18 16:52:11 thib Exp $

PROG=	fstest

clean:
	-umount /dev/vnd1c
	-umount /dev/vnd0c
	-vnconfig -u vnd1
	-vnconfig -u vnd0
	-rm -f ${.OBJDIR}/fakedisk
	-rm -f ${.OBJDIR}/fstest
	-rm -f ${.OBJDIR}/fstest.o

chflags: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/chflags/*.t

chmod: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/chmod/*.t

chown: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/chown/*.t
	
link: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/link/*.t

mkdir: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/mkdir/*.t

mkfifo: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/mkfifo/*.t

open: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/open/*.t

rename: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/rename/*.t

rmdir: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/rmdir/*.t

symlink: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/symlink/*.t

truncate: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/truncate/*.t

unlink: ${PROG}
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/unlink/*.t

run-regress-fstest:
	cd /mnt && env FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run\
		${.CURDIR}/tests/*/*.t

.include <bsd.regress.mk>
