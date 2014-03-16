# $FreeBSD$

completeinstall: _installlinks
.ORDER: afterinstall _installlinks
_installlinks:
.if defined(LINKS) && !empty(LINKS)
.  for dest lnk in ${LINKS}
	@l=${DESTDIR}${lnk}; \
	 d=${DESTDIR}${dest}; \
	 echo $$d -\> $$l; \
	 rm -f $$l; ln $$d $$l
.  endfor
.endif
.if defined(SYMLINKS) && !empty(SYMLINKS)
.  for lnk file in ${SYMLINKS}
	l=${lnk}; \
	 t=${DESTDIR}${file}; \
	 echo $$t -\> $$l; \
	 ln -sf $$l $$t;
.  endfor
.endif
