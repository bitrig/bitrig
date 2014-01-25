# written by pedro martelletto in january 2014; public domain

set -e

# kqtest-vnode [-d] <file> <fflag1,fflag2,...> <cmd>

ok() {
	echo kqtest-vnode "$@"
	kqtest-vnode "$@"
}

notok() {
	echo kqtest-vnode "$@"
	kqtest-vnode "$@" && exit 1
	echo "(expected)"
}

###############################################################################
# EVFILT_VNODE tests on files
###############################################################################

# the creation of a hardlink triggers NOTE_LINK.
# however, the removal of a hardlink does not trigger NOTE_LINK.
# a rename triggers NOTE_RENAME on the file.

ok	x NOTE_LINK			ln x y
notok	x NOTE_LINK			rm y
ok	x NOTE_RENAME			mv x y; rm -f y

# a write extending the file triggers NOTE_EXTEND.
# a write not extending the file does not trigger NOTE_EXTEND.

ok	x NOTE_WRITE,NOTE_EXTEND	sh -c 'echo y > x'
notok	x NOTE_WRITE			sh -c 'echo y > x'
ok	x NOTE_WRITE			sh write x z 0 # y -> z
notok	x NOTE_WRITE,NOTE_EXTEND	sh write x z 0 # y -> z

# setattr triggers NOTE_ATTRIB for everything.
# truncation to shorter length triggers NOTE_TRUNCATE.

ok	x NOTE_ATTRIB			chmod 0 x; rm -f x
ok	x NOTE_ATTRIB			chgrp nobody x; rm -f x
ok	x NOTE_ATTRIB			chflags uchg x; chflags nouchg x
ok	x NOTE_ATTRIB			/bin/sh -c 'sleep 3; touch x'
ok	x NOTE_ATTRIB			truncate x 100
ok	x NOTE_ATTRIB			truncate x 100
ok	x NOTE_ATTRIB,NOTE_TRUNCATE	truncate x 0

ok	x NOTE_REVOKE			revoke x

# the deletion of a file through unlink() or rename() triggers NOTE_DELETE.

ok	x NOTE_DELETE			/bin/sh -c 'touch y; mv y x'
ok	x NOTE_DELETE			rm x

###############################################################################
# EVFILT_VNODE tests on directories
###############################################################################

# creation of a {file,device,link} triggers NOTE_WRITE on the directory.
# creation of a directory triggers NOTE_WRITE,NOTE_LINK on the subdirectory.

ok	-d x NOTE_WRITE			touch x/x1
ok	-d x NOTE_WRITE			mknod x/x2 b 0 0
ok	-d x NOTE_WRITE			/bin/sh -c 'touch xx; ln xx x/x3'
ok	-d x NOTE_WRITE,NOTE_LINK	mkdir x/x4

# a rename from a directory triggers NOTE_WRITE on the directory.
# a rename to a directory triggers NOTE_WRITE on the directory.

ok	-d x NOTE_WRITE			mv x/x1 x/x4
ok	-d x NOTE_WRITE			mv x/x4/x1 x

# removal of a {file,device,link} triggers NOTE_WRITE on the directory.
# removal of a directory triggers NOTE_WRITE,NOTE_LINK on the subdirectory.
# removal of a directory triggers NOTE_DELETE.

ok	-d x NOTE_WRITE			rm -rf x/x1
ok	-d x NOTE_WRITE			rm -rf x/x2
ok	-d x NOTE_WRITE			rm -rf x/x3
ok	-d x NOTE_WRITE,NOTE_LINK	rm -rf x/x4
ok	-d x NOTE_DELETE		rm -rf x
