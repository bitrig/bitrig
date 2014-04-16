# $FreeBSD: src/tools/regression/fstest/tests/truncate/10.t,v 1.1 2007/01/17 01:42:12 pjd Exp $

desc="truncate returns EROFS if the named file resides on a read-only file system"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
mountfs ${n0}
expect 0 create ${n0}/${n1} 0644
expect 0 truncate ${n0}/${n1} 123
expect 123 stat ${n0}/${n1} size
mount -ur ${n0}
expect EROFS truncate ${n0}/${n1} 1234
expect 123 stat ${n0}/${n1} size
mount -uw ${n0}
expect 0 truncate ${n0}/${n1} 1234
expect 1234 stat ${n0}/${n1} size
expect 0 unlink ${n0}/${n1}
umountfs ${n0}
expect 0 rmdir ${n0}
