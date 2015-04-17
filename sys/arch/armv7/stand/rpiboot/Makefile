.if ${MACHINE} == "armv7"
PROG=		rpiboot
SRCS=		rpi.S

BINDIR=		/usr/mdec
OBJCOPY?=	objcopy
INSTALL_STRIP=

${PROG}: ${OBJS}
	${OBJCOPY} ${OBJS} -O binary ${PROG}
.else
NOPROG=
.endif

NOMAN=

.include <bsd.prog.mk>
