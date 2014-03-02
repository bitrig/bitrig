# $FreeBSD$

CFLAGS+=	-I/usr/local/include
LDFLAGS+=	-L/usr/local/lib

TESTS=	test-wcscasecmp

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}
