#	$OpenBSD: Makefile,v 1.1 2002/01/31 15:33:36 art Exp $
PROG=prog

A_DIR=${.OBJDIR}/../liba
A_OBJDIR!=    if [ -d $(A_DIR)/${__objdir} -o -h $(A_DIR)/${__objdir} ]; then \
			echo "$(A_DIR)/${__objdir}"; \
		else \
			echo "$(A_DIR)"; \
		fi


LDADD+=-L$(A_OBJDIR) -la
LDFLAGS+= -Wl,-rpath,$(A_OBJDIR)

.include <bsd.regress.mk>
