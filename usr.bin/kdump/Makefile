#	$OpenBSD: Makefile,v 1.25 2013/11/22 07:50:36 mpi Exp $

LDSTATIC=${STATIC}
PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR} -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c kdump_subr.c subr.c ktrstruct.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c kdump_subr.c

CROSSDIR?=

ioctl.c: ${.CURDIR}/Makefile ${.CURDIR}/mkioctls
	CC="${CC}" \
	/bin/sh ${.CURDIR}/mkioctls ${CROSSDIR}/usr/include/sys/ioctl.h \
	    ${CROSSDIR}/usr/include/sys/ioctl_compat.h \
	    ${CROSSDIR}/usr/include/crypto/cryptodev.h \
	    ${CROSSDIR}/usr/include/dev/biovar.h \
	    ${CROSSDIR}/usr/include/dev/systrace.h \
	    ${CROSSDIR}/usr/include/dev/wscons/wsconsio.h \
	    ${CROSSDIR}/usr/include/dev/vndioctl.h \
	    ${CROSSDIR}/usr/include/dev/pci/drm/drm.h \
	    ${CROSSDIR}/usr/include/dev/usb/usb.h \
	    ${CROSSDIR}/usr/include/net/bpf.h \
	    ${CROSSDIR}/usr/include/net/if_ppp.h \
	    ${CROSSDIR}/usr/include/net/if_pppoe.h \
	    ${CROSSDIR}/usr/include/net/if_tun.h \
	    ${CROSSDIR}/usr/include/net/pfvar.h \
	    ${CROSSDIR}/usr/include/net80211/ieee80211_ioctl.h \
	    ${CROSSDIR}/usr/include/netinet6/in6_var.h \
	    ${CROSSDIR}/usr/include/sys/tty.h \
	    ${CROSSDIR}/usr/include/sys/agpio.h \
	    ${CROSSDIR}/usr/include/sys/ataio.h \
	    ${CROSSDIR}/usr/include/sys/audioio.h \
	    ${CROSSDIR}/usr/include/sys/cdio.h \
	    ${CROSSDIR}/usr/include/sys/chio.h \
	    ${CROSSDIR}/usr/include/sys/dkio.h \
	    ${CROSSDIR}/usr/include/sys/filio.h \
	    ${CROSSDIR}/usr/include/sys/gpio.h \
	    ${CROSSDIR}/usr/include/sys/mtio.h \
	    ${CROSSDIR}/usr/include/sys/pciio.h \
	    ${CROSSDIR}/usr/include/sys/radioio.h \
	    ${CROSSDIR}/usr/include/sys/scanio.h \
	    ${CROSSDIR}/usr/include/sys/scsiio.h \
	    ${CROSSDIR}/usr/include/sys/sockio.h \
	    ${CROSSDIR}/usr/include/sys/videoio.h > ioctl.c

kdump_subr.c: ${.CURDIR}/mksubr
	sh ${.CURDIR}/mksubr ${CROSSDIR}/usr/include > ${.TARGET}

.include <bsd.prog.mk>
