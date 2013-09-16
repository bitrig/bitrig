#	$OpenBSD: Makefile.inc,v 1.2 2013/09/07 23:04:44 patrick Exp $

ALLSOC=IMX OMAP
KERNELS=
MDEXT=

.for SOC in ${ALLSOC}
KERNELS+=	GENERIC-${SOC}
MDEXT+=		bsd.${SOC}.umg
MDEXT+=		bsd.rd.${SOC}
MDEXT+=		bsd.rd.${SOC}.umg
.endfor


LOADADDR_OMAP=0x82800000
LOADADDR_IMX=0x10800000

# generating kernel umg files here in 'bootblocks' is a horrible hack, but 
# where is a better place?
bootblocks:
.for SOC in ${ALLSOC}
	mkuboot -a arm -o linux -e ${LOADADDR_${SOC}} -l ${LOADADDR_${SOC}} \
	    ${RELEASEDIR}/GENERIC-${SOC} ${RELEASEDIR}/GENERIC-${SOC}.umg
.endfor

MDEXT+=	miniroot-am335x-${OSrev}.fs miniroot-beagle-${OSrev}.fs \
	miniroot-panda-${OSrev}.fs
