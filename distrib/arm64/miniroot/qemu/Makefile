BOARD=	qemu
PLATFORM=qemu
LOADADDR=0x40800000
NBLKS=		32130
PART_SZ != expr ${NBLKS} - 2048

.include "${.CURDIR}/../Makefile.inc"

rd_setup_md:
	echo PART_SIZE = ${PART_SZ}
	fdisk -c 2 -h 255 -s 63 -yi ${VND} >/dev/null
	echo "u\ne 3\n0\ne 0\n${PART_ID}\nn\n2048\n${PART_SZ}\nf 0\nw\nq\n" | fdisk -c 2 -h 255 -s 63 -e ${VND} >/dev/null

