# $FreeBSD: src/tools/regression/fstest/tests/symlink/11.t,v 1.1 2007/01/17 01:42:11 pjd Exp $

desc="symlink returns ENOSPC if there are no free inodes on the file system on which the symbolic link is being created"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
mountfs_small ${n0}

i=0
while :; do
	if ! ln -s test ${n0}/${i} >/dev/null 2>&1 ; then
		break
	fi
	i=`expr $i + 1`
done
expect ENOSPC symlink test ${n0}/${n1}
umountfs ${n0}
expect 0 rmdir ${n0}
