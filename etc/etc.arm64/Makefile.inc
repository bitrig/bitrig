#	$OpenBSD: Makefile.inc,v 1.4 2013/11/01 20:35:52 jasper Exp $

ALLSOC=QEMU HIMEM
KERNELS=GENERIC bsd
MDEXT=bsd.rd

.for SOC in ${ALLSOC}
MDEXT+=		bsd.${SOC}.umg
MDEXT+=		bsd.rd.${SOC}.umg
.endfor

KERNADDR_QEMU=0x40080000
KERNADDR_HIMEM=0x8000000000

post-kernels:
.for SOC in ${ALLSOC}
	mkuboot -a arm -o linux -e ${KERNADDR_${SOC}} -l ${KERNADDR_${SOC}} \
	    ${RELEASEDIR}/bsd ${RELEASEDIR}/bsd.${SOC}.umg
.endfor

bootblocks:
