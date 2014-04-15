# $FreeBSD: src/tools/regression/fstest/tests/truncate/10.t,v 1.1 2007/01/17 01:42:12 pjd Exp $

desc="truncate returns EROFS if the named file resides on a read-only file system"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
dd if=/dev/zero of=tmpdisk bs=1k count=1024 2>/dev/null
vnconfig vnd1 tmpdisk
newfs /dev/rvnd1c >/dev/null
mountfs /dev/vnd1c ${n0}
expect 0 create ${n0}/${n1} 0644
expect 0 truncate ${n0}/${n1} 123
expect 123 stat ${n0}/${n1} size
mountfs -ur ${n0}
expect EROFS truncate ${n0}/${n1} 1234
expect 123 stat ${n0}/${n1} size
mountfs -uw ${n0}
expect 0 truncate ${n0}/${n1} 1234
expect 1234 stat ${n0}/${n1} size
expect 0 unlink ${n0}/${n1}
umount ${n0}
vnconfig -u vnd1
rm tmpdisk
expect 0 rmdir ${n0}
