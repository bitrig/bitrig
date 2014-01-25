echo -n "$2" | dd of=$1 bs=1 seek=$3 conv=notrunc 2>/dev/null 1>&2
