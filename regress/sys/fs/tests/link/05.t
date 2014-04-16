# $FreeBSD: src/tools/regression/fstest/tests/link/05.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="link returns EMLINK if the link count of the file named by name1 would exceed 32767"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
mountfs ${n0}
expect 0 create ${n0}/${n1} 0644
i=1
while :; do
	if ! ln ${n0}/${n1} ${n0}/${i} 2>/dev/null 1>&2 ; then
		break
	fi
	i=`expr $i + 1`
done
test_check $i -eq 32767

expect EMLINK link ${n0}/${n1} ${n0}/${n2}

umountfs ${n0}
expect 0 rmdir ${n0}
