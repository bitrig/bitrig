#!/bin/ksh
set -eu

MOUNT_POINT=/mnt
SNAPFILE=tmp.fs

ntests=0
nfail=0

mksock() {
	nc -l -U $1 &
	p=$!

	# XXX: nc might take some time to create the socket file.
	while [ ! -S $1 ]; do
		sleep 1
	done

	kill -KILL $p
}

validate_magic() {
	dd if=$1 of=$1.magic count=1 bs=7 > /dev/null 2>&1
	printf 'tmpfs\n\0' | cmp -s $1.magic -
	r=$?
	rm $1.magic
	return $r
}

check() {
	(( ntests += 1 ))
	echo -n " - $1: "
	if (eval "$2"); then
		echo 'ok'
	else
		(( nfail += 1 ))
		echo 'fail'
	fi
}

report() {
	echo "\nTests: ${ntests}, Failed: ${nfail}"
	[ $nfail -eq 0 ]
	exit
}

mkfiles() {
	# File Types
	echo foobar > $1/reg
	(cd $1; ln -s reg symlink)
	ln $1/reg $1/hardlink
	mkdir $1/dir
	mknod $1/blk b 0 0
	mknod $1/chr c 2 12
	mkfifo $1/fifo
	mksock $1/sock

	# Timestamps
	touch -am -d 1900-01-01T00:00:00Z $1/1900
	touch -am -d 1970-01-01T00:00:00Z $1/1970
	touch -am -d 2100-01-01T00:00:00Z $1/2100

	# Owner
	touch $1/owner
	chown 42:42 $1/owner

	# Permissions
	for i in 1 2 4; do
		for perm in 000${i} 00${i}0 0${i}00 ${i}000; do
			touch $1/$perm
			chmod $perm $1/$perm
		done
	done

        # File flags
	for flg in arch nodump sappnd schg uappnd uchg; do
		touch $1/$flg
		chflags $flg $1/$flg
	done
}

mount -t tmpfs tmpfs ${MOUNT_POINT}

mkfiles ${MOUNT_POINT}

rm -f ${SNAPFILE}

#
# Snapshot
#

echo "Snapshot:"

check dump \
    'mount -t tmpfs -u -o snapshot ${SNAPFILE} ${MOUNT_POINT}'
umount ${MOUNT_POINT}

check 'not empty' \
    '[ -s ${SNAPFILE} ]'

check magic \
    'validate_magic ${SNAPFILE}'

check restore \
    'mount -t tmpfs ${SNAPFILE} ${MOUNT_POINT}'


#
# File Types
#

echo "\nTypes:"

check regular \
    '[ -f ${MOUNT_POINT}/reg ]'

check 'regular (content)' \
    'echo foobar | diff - ${MOUNT_POINT}/reg'

check symlink \
    '[ -h ${MOUNT_POINT}/symlink ]'

check 'symlink (target)' \
    '[ `readlink ${MOUNT_POINT}/symlink` = reg ]'

check hardlink \
    '[ -f ${MOUNT_POINT}/hardlink ]'

check 'hardlink (inode number)' \
    '[ `stat -f %i ${MOUNT_POINT}/reg` = `stat -f %i ${MOUNT_POINT}/hardlink` ]'

check directory \
    '[ -d ${MOUNT_POINT}/dir ]'

check 'block device' \
    '[ -b ${MOUNT_POINT}/blk ]'

check 'block device (major, minor)' \
    '[ `stat -f %Z ${MOUNT_POINT}/blk` = 0,0 ]'

check 'character device' \
    '[ -c ${MOUNT_POINT}/chr ]'

check 'character device (major, minor)' \
    '[ `stat -f %Z ${MOUNT_POINT}/chr` = 2,12 ]'

check fifo \
    '[ -p ${MOUNT_POINT}/fifo ]'

check socket \
    '[ -S ${MOUNT_POINT}/sock ]'


#
# Timestamps
#
# XXX: how to test ctime?

echo "\nTimestamps:"

check 'atime (< epoch)' \
    '[ `stat -f %a ${MOUNT_POINT}/1900` -eq -2208988800 ]'

check 'atime (== epoch)' \
    '[ `stat -f %a ${MOUNT_POINT}/1970` -eq 0 ]'

check 'atime (> epoch)' \
    '[ `stat -f %a ${MOUNT_POINT}/2100` -eq 4102444800 ]'

check 'mtime (< epoch)' \
    '[ `stat -f %m ${MOUNT_POINT}/1900` -eq -2208988800 ]'

check 'mtime (== epoch)' \
    '[ `stat -f %m ${MOUNT_POINT}/1970` -eq 0 ]'

check 'mtime (> epoch)' \
    '[ `stat -f %m ${MOUNT_POINT}/2100` -eq 4102444800 ]'


#
# Owner
#

echo "\nOwner:"

check uid \
    '[ `stat -f %u ${MOUNT_POINT}/owner` -eq 42 ]'

check gid \
    '[ `stat -f %g ${MOUNT_POINT}/owner` -eq 42 ]'


#
# Permissions
#

echo "\nPermissions:"

for i in 1 2 4; do
	for perm in 000${i} 00${i}0 0${i}00 ${i}000; do
		check $perm \
		    '[ `stat -f %p ${MOUNT_POINT}/$perm` = 10$perm ]'
	done
done


#
# File Flags
#
# See /usr/include/sys/stat.h for numerical values.

echo "\nFlags:"

check arch \
    '[ `stat -f %f ${MOUNT_POINT}/arch` -eq $(( 0x00010000 )) ]'

check nodump \
    '[ `stat -f %f ${MOUNT_POINT}/nodump` -eq $(( 0x00000001 )) ]'

check sappnd \
    '[ `stat -f %f ${MOUNT_POINT}/sappnd` -eq $(( 0x00040000 )) ]'

check schg \
    '[ `stat -f %f ${MOUNT_POINT}/schg` -eq $(( 0x00020000 )) ]'

check uappnd \
    '[ `stat -f %f ${MOUNT_POINT}/uappnd` -eq $(( 0x00000004 )) ]'

check uchg \
    '[ `stat -f %f ${MOUNT_POINT}/uchg` -eq $(( 0x00000002 )) ]'


umount ${MOUNT_POINT}
rm ${SNAPFILE}

report
