# $FreeBSD: src/tools/regression/fstest/tests/mkdir/09.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkdir returns EROFS if the named file resides on a read-only file system"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
mountfs ${n0}
expect 0 mkdir ${n0}/${n1} 0755
expect 0 rmdir ${n0}/${n1}
mount -ur ${n0}
expect EROFS mkdir ${n0}/${n1} 0755
mount -uw ${n0}
expect 0 mkdir ${n0}/${n1} 0755
expect 0 rmdir ${n0}/${n1}
umountfs ${n0}
expect 0 rmdir ${n0}
