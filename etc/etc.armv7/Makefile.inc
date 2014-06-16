#	$OpenBSD: Makefile.inc,v 1.4 2013/11/01 20:35:52 jasper Exp $

ALLSOC=IMX OMAP EXYNOS SUNXI
KERNELS=GENERIC bsd
MDEXT=bsd.rd

.for SOC in ${ALLSOC}
MDEXT+=		bsd.${SOC}.umg
MDEXT+=		bsd.rd.${SOC}.umg
.endfor

KERNADDR_EXYNOS=0x40300000
KERNADDR_IMX=0x10300000
KERNADDR_OMAP=0x80300000
KERNADDR_SUNXI=0x40300000

post-kernels:
.for SOC in ${ALLSOC}
	mkuboot -a arm -o linux -e ${KERNADDR_${SOC}} -l ${KERNADDR_${SOC}} \
	    ${RELEASEDIR}/bsd ${RELEASEDIR}/bsd.${SOC}.umg
.endfor

bootblocks:

MDEXT+=	miniroot-am335x-${OSrev}.fs miniroot-beagle-${OSrev}.fs \
	miniroot-panda-${OSrev}.fs
