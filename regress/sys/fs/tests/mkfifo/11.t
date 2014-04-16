# $FreeBSD: src/tools/regression/fstest/tests/mkfifo/11.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkfifo returns ENOSPC if there are no free inodes on the file system on which the file is being created"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
mountfs_small ${n0}
i=0
while :; do
	if ! mkfifo ${n0}/${i} >/dev/null 2>&1 ; then
		break
	fi
	i=`expr $i + 1`
done
expect ENOSPC mkfifo ${n0}/${n1} 0644
umountfs ${n0}
expect 0 rmdir ${n0}
