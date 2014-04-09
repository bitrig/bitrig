#	$OpenBSD: Makefile,v 1.88 2014/03/18 22:36:31 miod Exp $
#	$NetBSD: Makefile,v 1.13 1996/03/28 21:36:40 mark Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN=	afterboot.8 autoinstall.8 boot_config.8 \
	crash.8 daily.8 \
	diskless.8 genassym.sh.8 intro.8 netstart.8 rc.8 \
	rc.conf.8 rc.d.8 rc.shutdown.8 rc.subr.8 release.8 \
	security.8 ssl.8 starttls.8 sticky.8 yp.8

MLINKS+=boot_config.8 UKC.8
MLINKS+=daily.8 weekly.8 daily.8 monthly.8
MLINKS+=rc.8 rc.firsttime.8 rc.8 rc.local.8 rc.8 rc.securelevel.8
MLINKS+=rc.conf.8 rc.conf.local.8

SUBDIR=	man8.amd64 man8.armv7

.include <bsd.prog.mk>
