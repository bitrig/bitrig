# $FreeBSD: src/tools/regression/fstest/tests/symlink/10.t,v 1.1 2007/01/17 01:42:11 pjd Exp $

desc="symlink returns EROFS if the file name2 would reside on a read-only file system"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
dd if=/dev/zero of=tmpdisk bs=1k count=1024 2>/dev/null
vnconfig vnd1 tmpdisk
newfs /dev/rvnd1c >/dev/null
mountfs /dev/vnd1c ${n0}

expect 0 symlink test ${n0}/${n1}
expect 0 unlink ${n0}/${n1}
mountfs -ur ${n0}
expect EROFS symlink test ${n0}/${n1}
mountfs -uw ${n0}
expect 0 symlink test ${n0}/${n1}
expect 0 unlink ${n0}/${n1}

umount ${n0}
vnconfig -u vnd1
rm tmpdisk
expect 0 rmdir ${n0}
