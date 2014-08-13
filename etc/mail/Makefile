MAKEMAP = /usr/libexec/smtpd/makemap

distribution:
	${INSTALL} -c -o root -g wheel -m 644 Makefile \
	    ${DESTDIR}/etc/mail/Makefile
	${INSTALL} -c -o root -g wheel -m 644 aliases \
	    ${DESTDIR}/etc/mail/aliases
	${MAKEMAP} -o ${DESTDIR}/etc/mail/aliases.db aliases
	chown root:wheel ${DESTDIR}/etc/mail/aliases.db
	chmod 644 ${DESTDIR}/etc/mail/aliases.db
	${INSTALL} -c -o root -g wheel -m 644 spamd.conf \
	    ${DESTDIR}/etc/mail/spamd.conf
	${INSTALL} -c -o root -g wheel -m 644 smtpd.conf \
	    ${DESTDIR}/etc/mail/smtpd.conf

.PHONY: all
.include <bsd.own.mk>
.include <bsd.sys.mk>
