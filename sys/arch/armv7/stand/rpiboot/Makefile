.if ${MACHINE} == "armv7"
PROG=		rpiboot
SRCS=		rpi.S
OBJCOPY?=	objcopy

${PROG}: ${OBJS}
	${OBJCOPY} ${OBJS} -O binary ${PROG}
.else
NOPROG=
.endif

NOMAN=

.include <bsd.prog.mk>
