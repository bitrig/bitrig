# $FreeBSD: src/tools/regression/fstest/tests/rmdir/13.t,v 1.1 2007/01/17 01:42:11 pjd Exp $

desc="rmdir returns EBUSY if the directory to be removed is the mount point for a mounted file system"

n0=`namegen`

expect 0 mkdir ${n0} 0755
mountfs ${n0}
expect EBUSY rmdir ${n0}
umountfs ${n0}
expect 0 rmdir ${n0}
