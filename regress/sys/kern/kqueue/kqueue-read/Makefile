truncate: truncate.c
kqtest-read: kqtest-read.c

run:	kqtest-read truncate
	sh ${.CURDIR}/run.sh

REGRESS_TARGETS= run
CLEANFILES+= kqtest-read truncate

clean:
	rm -f ${CLEANFILES}
	rm -rf x xx y

.PATH:	${.CURDIR}/../kqueue-vnode

.include <bsd.regress.mk>
