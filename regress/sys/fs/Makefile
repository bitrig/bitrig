PROG=	fstest

TARGETS= ffs ffs-wapbl tmpfs

clean:
.for T in ${TARGETS}
	/bin/sh ${.CURDIR}/clean ${.CURDIR}/${T}
.endfor
	-rm -f *.o ${PROG} *.log

.for T in ${TARGETS}
test-${T}: ${T} ${PROG}
	FSTEST=${.OBJDIR}/fstest /bin/sh ${.CURDIR}/run ${.CURDIR}/${T}\
		${.CURDIR}/tests/*/*.t
.endfor

.include <bsd.regress.mk>
