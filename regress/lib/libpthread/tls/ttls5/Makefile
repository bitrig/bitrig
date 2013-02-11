# $FreeBSD$

PROG=	ttls5
DPADD=	${LIBPTHREAD}
LDADD=	-lpthread
LDFLAGS+= -L../libzz -lzz -Wl,--rpath=${.OBJDIR}/../libzz

CFLAGS+= -I${.CURDIR}/../libzz
NOMAN=
DEBUG_FLAGS= -g

.include <bsd.regress.mk>
