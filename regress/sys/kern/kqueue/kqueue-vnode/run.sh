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
}
