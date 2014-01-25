revoke: revoke.c
truncate: truncate.c
kqtest-vnode: kqtest-vnode.c

write: write.sh
	cp ${.CURDIR}/write.sh ${.OBJDIR}/write

run:	kqtest-vnode revoke truncate write
	PATH=$${PATH}:${.OBJDIR} sh ${.CURDIR}/run.sh

REGRESS_TARGETS= run
CLEANFILES+= kqtest-vnode revoke truncate write

clean:
	rm -f ${CLEANFILES}
	rm -rf x xx y

.include <bsd.regress.mk>
