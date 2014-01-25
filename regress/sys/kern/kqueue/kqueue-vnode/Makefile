revoke: revoke.c
truncate: truncate.c
kqtest-vnode: kqtest-vnode.c

run:	kqtest-vnode revoke truncate
	PATH=$${PATH}:${.OBJDIR} sh ${.CURDIR}/run.sh

REGRESS_TARGETS= run
CLEANFILES+= kqtest-vnode revoke truncate

clean:
	rm -f ${CLEANFILES}
	rm -rf x xx y

.include <bsd.regress.mk>
