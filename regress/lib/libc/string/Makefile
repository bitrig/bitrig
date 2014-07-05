# $FreeBSD$

CFLAGS+=	-I/usr/local/include
LDFLAGS+=	-L/usr/local/lib

TESTS=	test-wcscasecmp test-ungetwc

REGRESS_TARGETS=tests

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}

.include <bsd.regress.mk>
