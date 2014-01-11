# options for all the directories below

.if ${COMPILER_VERSION:L} == "gcc4"
COPTS+=        -Os
.else
COPTS+=        -Oz
.endif
COPTS+=-fno-stack-protector

MAN=
LDSTATIC=-static
NOPIE=
NOMAN=1
