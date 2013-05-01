MAKEMAP = /usr/libexec/smtpd/makemap

distribution: aliases.db
	${INSTALL} -c -o root -g wheel -m 644 Makefile \
	    ${DESTDIR}/etc/mail/Makefile
	${INSTALL} -c -o root -g wheel -m 644 aliases \
	    ${DESTDIR}/etc/mail/aliases
	${INSTALL} -c -o root -g wheel -m 644 aliases.db \
	    ${DESTDIR}/etc/mail/aliases.db
	${INSTALL} -c -o root -g wheel -m 644 spamd.conf \
	    ${DESTDIR}/etc/mail/spamd.conf
	${INSTALL} -c -o root -g wheel -m 644 smtpd.conf \
	    ${DESTDIR}/etc/mail/smtpd.conf

aliases.db: aliases
	${MAKEMAP} ${.ALLSRC}

.PHONY: all
.include <bsd.own.mk>
.include <bsd.sys.mk>
