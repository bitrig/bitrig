revoke: revoke.c
kqtest: kqtest.c

run:	kqtest revoke
	sh ${.CURDIR}/run.sh ${.OBJDIR}

REGRESS_TARGETS= run
CLEANFILES+= kqtest revoke x y

.include <bsd.regress.mk>
