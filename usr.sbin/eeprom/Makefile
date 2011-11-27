#	$OpenBSD: Makefile,v 1.12 2007/11/14 20:38:32 kettenis Exp $

.if ${MACHINE} == "macppc"
PROG=	eeprom

BINGRP=	kmem
BINMODE=2555

SRCS=	getdate.c main.c
SRCS+=	ophandlers.c optree.c

CLEANFILES+=getdate.c y.tab.h
.else
NOPROG=yes
.endif

MAN=	eeprom.8
MANSUBDIR=macppc

.include <bsd.prog.mk>
