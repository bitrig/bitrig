# $FreeBSD: src/tools/regression/fstest/tests/mkdir/11.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkdir returns ENOSPC if there are no free inodes on the file system on which the directory is being created"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
mountfs_small ${n0}
i=0
while :; do
	if ! mkdir ${n0}/${i} >/dev/null 2>&1 ; then
		break
	fi
	i=`expr $i + 1`
done
expect ENOSPC mkdir ${n0}/${n1} 0755
umountfs ${n0}
expect 0 rmdir ${n0}
