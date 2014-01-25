# written by pedro martelletto in january 2014; public domain
set -e

ok() {
	echo kqtest-vnode "$@"
	kqtest-vnode "$@"
}

notok() {
	echo kqtest-vnode "$@"
	kqtest-vnode "$@" && exit 1
	echo "(expected)"
}

# kqtest-vnode [-d] <file> <ev1,ev2,...> <cmd>

# EVFILT_VNODE tests on files
ok	x NOTE_LINK			ln x y
notok	x NOTE_LINK			rm y
ok	x NOTE_RENAME			mv x y; rm -f y
ok	x NOTE_ATTRIB			chmod 0 x; rm -f x
ok	x NOTE_WRITE,NOTE_EXTEND	sh -c 'echo y > x'
ok	x NOTE_WRITE			sh write.sh x z 0 # y -> z
ok	x NOTE_ATTRIB,NOTE_TRUNCATE	truncate x 0
ok	x NOTE_ATTRIB			truncate x 100
ok	x NOTE_ATTRIB			truncate x 100
ok	x NOTE_REVOKE			revoke x
ok	x NOTE_DELETE			rm x

touch xx

# EVFILT_VNODE tests on directories
ok	-d x NOTE_WRITE			touch x/x1
ok	-d x NOTE_WRITE,NOTE_LINK	mkdir x/x2
ok	-d x NOTE_WRITE			ln xx x/x3
ok	-d x NOTE_WRITE			rm -rf x/x1
ok	-d x NOTE_WRITE,NOTE_LINK	rm -rf x/x2
ok	-d x NOTE_WRITE			rm -rf x/x3
ok	-d x NOTE_REVOKE		revoke x
ok	-d x NOTE_RENAME		mv x y
ok	-d y NOTE_ATTRIB		chmod 0 y
ok	-d y NOTE_ATTRIB		chmod 0 y
ok	-d y NOTE_DELETE		rm -rf y
