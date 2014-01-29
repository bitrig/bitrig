BOARD=	exynos
PLATFORM=EXYNOS
FS=ext2fs
VND_FSDEV?=	${VND_JDEV}
VND_RFSDEV?=	${VND_RJDEV}

.include "${.CURDIR}/../Makefile.inc"

copy_md_files:
	ln ${MOUNT_POINT}/bsd.umg ${MOUNT_POINT}/uImage
	cp /usr/mdec/${BOARD}/*dtb ${MOUNT_POINT}
	dd if=/usr/mdec/${BOARD}/nv_uboot-snow-simplefb.kpart of=${VND_RIDEV} conv=osync

rd_setup_md:
	fdisk -c 2 -h 255 -s 63 -yig ${VND} >/dev/null
	gdisk -yi ${VND} >/dev/null
	echo 'e 0\n7f00\nKERN_A\n64\n16320\ne 2\n7f01\n\n16384\n\nf 0\nn\n8\n3\n1\nw\nq'| gdisk -e ${VND} >/dev/null
