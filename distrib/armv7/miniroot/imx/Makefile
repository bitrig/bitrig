BOARD=	imx
PLATFORM=IMX
LOADADDR=0x18800000
FS=ext2fs
PART_ID=83

.include "${.CURDIR}/../Makefile.inc"

copy_md_files:
	echo "; setenv loadaddr ${LOADADDR} ; setenv bootargs sd0i:/bsd.umg ; for dtype in sata mmc ; do for disk in 0 1 ; do \$${dtype} dev \$${disk} ; for fs in fat ext2 ; do if \$${fs}load \$${dtype} \$${disk}:1 \$${loadaddr} bsd.umg ; then  bootm \$${loadaddr} ; fi ; done; done; done; echo; echo failed to load bsd.umg" > 6x_bootscript.cmd
	${MKUBOOT} -t script -a arm -o linux 6x_bootscript.cmd 6x_bootscript.scr
	cp 6x_bootscript.scr ${MOUNT_POINT}/6x_bootscript

rd_setup_md:
	fdisk -c 2 -h 255 -s 63 -yi ${VND} >/dev/null
	echo "u\ne 3\n0\ne 0\n${PART_ID}\ny\n0\n1\n1\n1\n254\n63\nf 0\nw\nq\n" | fdisk -c 2 -h 255 -s 63 -e ${VND} >/dev/null
