# $FreeBSD: src/tools/regression/fstest/tests/mkfifo/11.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkfifo returns ENOSPC if there are no free inodes on the file system on which the file is being created"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
COUNT=256
NEWFSARGS=
# Leave space for the log.
[ "${WAPBL}" ] && { COUNT=2048; NEWFSARGS="-s -1792k"; }
dd if=/dev/zero of=tmpdisk bs=1k count=${COUNT} 2>/dev/null
vnconfig vnd1 tmpdisk
newfs ${NEWFSARGS} /dev/rvnd1c >/dev/null
mountfs /dev/vnd1c ${n0}
i=0
while :; do
	if ! mkfifo ${n0}/${i} >/dev/null 2>&1 ; then
		break
	fi
	i=`expr $i + 1`
done
expect ENOSPC mkfifo ${n0}/${n1} 0644
umount ${n0}
vnconfig -u vnd1
rm tmpdisk
expect 0 rmdir ${n0}
