BOARD=	sunxi
PLATFORM=SUNXI
LOADADDR=0x40200000

.include "${.CURDIR}/../Makefile.inc"

copy_md_files:
	echo 'bootargs=sd0a:/bsd' > ${MOUNT_POINT}/uEnv.tx
	echo 'mmcboot=mmc rescan ; fatload mmc 0 ${LOADADDR} bsd.umg && bootm ${LOADADDR};' >> ${MOUNT_POINT}/uEnv.txt
	echo 'uenvcmd=run mmcboot;' >> ${MOUNT_POINT}/uEnv.txt
	dd if=/usr/mdec/sunxi/sunxi-spl.bin of=${VND_CDEV} bs=1024 seek=8
	dd if=/usr/mdec/sunxi/u-boot.bin of=${VND_CDEV} bs=1024 seek=32

rd_setup_md:
	fdisk -c 2 -h 255 -s 63 -yi ${VND} >/dev/null
	echo "u\ne 3\n0\ne 0\n${PART_ID}\ny\n0\n99\n1\n1\n254\n63\nf 0\nw\nq\n" | fdisk -c 2 -h 255 -s 63 -e ${VND} >/dev/null
