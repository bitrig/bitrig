# mlo must be first file on filesystem according to some comments.
copy_md_files_pre:
	cp /usr/mdec/${BOARD}/mlo ${MOUNT_POINT}/mlo

copy_md_files:
	echo "bootcmd=mmc rescan ; setenv loadaddr ${LOADADDR} ; setenv bootargs sd0i:/bsd.umg ; fatload mmc \$${mmcdev} \$${loadaddr} bsd.umg ; bootm \$${loadaddr} ;\nuenvcmd=boot" > ${MOUNT_POINT}/uenv.txt
	cp /usr/mdec/${BOARD}/u-boot.* ${MOUNT_POINT}/

rd_setup_md:
	fdisk -c 2 -h 255 -s 63 -yi ${VND} >/dev/null
	echo "u\ne 3\n0\ne 0\n${PART_ID}\ny\n0\n1\n1\n1\n254\n63\nf 0\nw\nq\n" | fdisk -c 2 -h 255 -s 63 -e ${VND} >/dev/null
