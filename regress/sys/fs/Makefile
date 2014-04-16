PROG=	fstest

TARGETS= ffs tmpfs

clean:
.for t in ${TARGETS}
	/bin/sh ${.CURDIR}/clean ${.CURDIR}/${t}
.endfor
	-rm -f *.o ${PROG} *.log

run-regress-fstest:
.for t in ${TARGETS}
	FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run ${.CURDIR}/${t}\
		${.CURDIR}/tests/*/*.t
.endfor

.include <bsd.regress.mk>
