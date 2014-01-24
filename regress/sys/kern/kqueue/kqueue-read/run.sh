# written by pedro martelletto in january 2014; public domain
set -e

ok() {
	expected=$1
	shift
	echo "./kqtest-read $@ | egrep ^$expected\$"
	./kqtest-read "$@" | egrep ^$expected\$ 2>/dev/null 1>&2
}

notok() {
	shift
	echo ./kqtest-read "$@"
	./kqtest-read "$@" && exit 1
	echo "(expected)"
}

# kqtest-read [-d] [-s seek] [file] <ev1,ev2,...> <cmd>

# EVFILT_READ tests on files
#	expected	arguments	command
ok	2		x 0		/bin/sh -c 'echo y > x'
ok	2		x 0		/bin/sh -c 'echo y > x'
ok	4		x 0		/bin/sh -c 'echo y >> x'
ok	3		-s 1 x 0	true
notok	0		-s 4 x 0	true
ok	0		-s 4 x NOTE_EOF	true
ok	-5		-s 9 x 0	true
ok	1024		x 0		./truncate x 1024
ok	503		-s 9 x 0	./truncate x 512
ok	-1792		-s 2048 x 0	./truncate x 256
ok	0		x 0		./truncate x 0; rm x
ok	2		x NOTE_EOF	/bin/sh -c 'echo y > x'
ok	2		x NOTE_EOF	/bin/sh -c 'echo y > x'
ok	0		x NOTE_EOF	./truncate x 0; rm x

# EVFILT_READ tests on directories
#	expected	arguments		command
ok	512		-d x 0			mkdir x/x
ok	1		-d -s 511 x 0		mkdir x/y
ok	512		-d x 0			touch x/x1
ok	1		-d -s 511 x 0		touch x/x2
notok	0		-d -s 512 x 0		true
ok	0		-d -s 512 x NOTE_EOF	true
ok	-1536		-d -s 2048 x 0		rm -r x/y
ok	0		-d x 0			rm -r x
