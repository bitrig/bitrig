#	$OpenBSD: Makefile,v 1.53 2011/06/23 22:46:12 schwarze Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c editor.c manual.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	disklabel.8 disklabel.5


CLEANFILES+=manual.c

.include <bsd.own.mk>

.ifdef NOMAN
manual.c:
	(echo 'const unsigned char manpage[] = {'; \
	echo 'no manual' | gzip -9c | hexdump -ve '"0x" 1/1 "%02x,"'; \
	echo '};'; echo 'const int manpage_sz = sizeof(manpage);') > manual.c
.else
disklabel.cat8:	disklabel.8
	mandoc ${.ALLSRC} > ${.TARGET}

manual.c:	disklabel.cat8
	(echo 'const unsigned char manpage[] = {'; \
	cat disklabel.cat8 | gzip -9c | hexdump -ve '"0x" 1/1 "%02x,"'; \
	echo '};'; echo 'const int manpage_sz = sizeof(manpage);') > manual.c
.endif

.if (${MACHINE} == "amd64") || (${MACHINE} == "i386")
CFLAGS+= -DSEEALSO="\"fdisk(8), installboot(8)\""
.endif

.if (${MACHINE} == "armish") || (${MACHINE} == "zaurus")
CFLAGS+= -DSEEALSO="\"fdisk(8)\""
.endif

.if (${MACHINE} == "macppc" || ${MACHINE} == "socppc")
CFLAGS+= -DSEEALSO="\"fdisk(8), pdisk(8)\""
.endif

.ifdef NOPIC
CFLAGS+= -DSTATICLINKING
.endif

.include <bsd.prog.mk>
