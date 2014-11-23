desc="after lstat on a symlink, st_size must be set to the length of the \
	pathname contained in the symlink, not including any terminating null \
	byte"

n0=`namegen`
n1=`namegen`

expect 0 create ${n0} 0644
expect 0 symlink ${n0} ${n1}
expect ${#n0} lstat ${n1} size
expect 0 unlink ${n1}
expect 0 unlink ${n0}
