set -x
cd $1 && {
	./kqtest x NOTE_LINK ln x y
	./kqtest x NOTE_LINK rm y && exit 1
	./kqtest x NOTE_RENAME mv x y; rm -f y
	./kqtest x NOTE_ATTRIB chmod 0 x; rm -f x
	./kqtest x NOTE_WRITE,NOTE_EXTEND /bin/sh -c 'echo y > x'
	./kqtest x NOTE_ATTRIB,NOTE_TRUNCATE dd if=/dev/zero of=x bs=1 count=0
	./kqtest x NOTE_ATTRIB dd if=/dev/zero of=x bs=1 seek=100 count=0
	./kqtest x NOTE_REVOKE ./revoke x
	./kqtest x NOTE_DELETE rm x
	./kqtest -d x NOTE_WRITE touch x/x1
	./kqtest -d x NOTE_WRITE,NOTE_LINK mkdir x/x2
	touch xx
	./kqtest -d x NOTE_WRITE ln xx x/x3
	./kqtest -d x NOTE_WRITE rm -rf x/x1
	./kqtest -d x NOTE_WRITE,NOTE_LINK rm -rf x/x2
	./kqtest -d x NOTE_WRITE rm -rf x/x3
	./kqtest -d x NOTE_REVOKE ./revoke x
	./kqtest -d x NOTE_RENAME mv x y
#	./kqtest -d x NOTE_ATTRIB chmod 0 y
#	./kqtest -d x NOTE_DELETE rm -rf y
}
