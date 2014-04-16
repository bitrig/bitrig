# $FreeBSD: src/tools/regression/fstest/tests/symlink/10.t,v 1.1 2007/01/17 01:42:11 pjd Exp $

desc="symlink returns EROFS if the file name2 would reside on a read-only file system"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
mountfs ${n0}

expect 0 symlink test ${n0}/${n1}
expect 0 unlink ${n0}/${n1}
mount -ur ${n0}
expect EROFS symlink test ${n0}/${n1}
mount -uw ${n0}
expect 0 symlink test ${n0}/${n1}
expect 0 unlink ${n0}/${n1}

umountfs ${n0}
expect 0 rmdir ${n0}
