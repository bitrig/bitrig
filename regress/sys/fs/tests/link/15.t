# $FreeBSD: src/tools/regression/fstest/tests/link/15.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="link returns ENOSPC if the directory in which the entry for the new link is being placed cannot be extended because there is no space left on the file system containing the directory"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
mountfs_small ${n0}
expect 0 create ${n0}/${n1} 0644
i=0
while :; do
	if ! ln ${n0}/${n1} ${n0}/${i} >/dev/null 2>&1 ; then
		break
	fi
	i=`expr $i + 1`
done
expect ENOSPC link ${n0}/${n1} ${n0}/${n2}
umountfs ${n0}
expect 0 rmdir ${n0}
