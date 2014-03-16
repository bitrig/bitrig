#	$OpenBSD: Makefile,v 1.12 2014/02/17 11:36:53 schwarze Exp $
#	from: @(#)Makefile	5.13 (Berkeley) 5/7/91

FILES=	airport ascii birthtoken countrycodes eqnchar getopt \
	inter.phone license.template mdoc.template \
	na.phone operator scsi_modes usb_hid_usages usb_hid_usages \
	zipcodes 

.if (${MACHINE} == "i386") || (${MACHINE} == "amd64") || \
    (${MACHINE} == "alpha")
SUBDIR=	pcvtfonts
.endif

all clean cleandir depend lint obj tags: _SUBDIRUSE

FILESDIR = ${BINDIR}/misc

.include <bsd.prog.mk>
