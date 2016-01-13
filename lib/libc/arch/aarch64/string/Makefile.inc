# $OpenBSD: Makefile.inc,v 1.6 2012/09/04 03:10:42 okan Exp $
# $NetBSD: Makefile.inc,v 1.5 2002/11/23 14:26:04 chris Exp $

SRCS+=  memcpy.c bcopy.c memmove.c memset.c bzero.c ffs.c strcmp.c
SRCS+=	strncmp.c memcmp.c
SRCS+=	strchr.c strrchr.c 
SRCS+=	bcmp.c memchr.c \
	strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c strlcpy.c strlcat.c
