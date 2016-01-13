BOARD=	qemu
PLATFORM=qemu
LOADADDR=0x40800000

.include "${.CURDIR}/../Makefile.inc"

rd_setup_md:
	fdisk -c 2 -h 255 -s 63 -yi ${VND} >/dev/null
	echo "u\ne 3\n0\ne 0\n${PART_ID}\nn\n2048\n\nf 0\nw\nq\n" | fdisk -c 2 -h 255 -s 63 -e ${VND} >/dev/null

