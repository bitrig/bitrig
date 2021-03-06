HDRDIR=		${.CURDIR}/../../contrib/libc++abi/include
UHDRDIR=	${.CURDIR}/../../contrib/libunwind/include
SRCDIR=		${.CURDIR}/../../contrib/libc++abi/src
.if !empty(CROSSDIR)
CXXINCLUDEDIR=	${CROSSDIR}/usr/include/c++/v1
.else
CXXINCLUDEDIR=	/usr/include/c++/v1
.endif

.PATH: ${SRCDIR} ${.CURDIR}/../../contrib/libunwind/src

LIB=		c++abi

SRCS+=		abort_message.cpp\
		cxa_aux_runtime.cpp\
		cxa_default_handlers.cpp\
		cxa_demangle.cpp\
		cxa_exception.cpp\
		cxa_exception_storage.cpp\
		cxa_guard.cpp\
		cxa_handlers.cpp\
		cxa_new_delete.cpp\
		cxa_personality.cpp\
		cxa_thread_atexit.cpp\
		cxa_unexpected.cpp\
		cxa_vector.cpp\
		cxa_virtual.cpp\
		exception.cpp\
		private_typeinfo.cpp\
		stdexcept.cpp\
		typeinfo.cpp\
		Unwind-EHABI.cpp\
		Unwind-sjlj.c\
		UnwindLevel1-gcc-ext.c\
		UnwindLevel1.c\
		UnwindRegistersRestore.S\
		UnwindRegistersSave.S\
		libunwind.cpp

WARNS=		0
CFLAGS+=	-nostdlib -I${CXXINCLUDEDIR} -funwind-tables
CPPFLAGS+=	${CFLAGS}
.if empty(CXXFLAGS:M-std=*)
CXXFLAGS+=	-std=c++11
.endif

STD_HEADERS=	cxxabi.h \
		__cxxabi_config.h

.for hdr in ${STD_HEADERS}
STD+=		${hdr} ${HDRDIR}/${hdr}
.endfor
STDDIR=		${CXXINCLUDEDIR}

# Libunwind stuff
STD_UHEADERS=	libunwind.h \
		__libunwind_config.h \
		unwind.h

.for hdr in ${STD_UHEADERS}
USTD+=		${hdr} ${UHDRDIR}/${hdr}
.endfor
USTDDIR=	${CXXINCLUDEDIR}

MACHO_HEADERS=compact_unwind_encoding.h

.for hdr in ${MACHO_HEADERS}
MACHO+=		${hdr} ${UHDRDIR}/mach-o/${hdr}
.endfor
MACHODIR=	${CXXINCLUDEDIR}/mach-o

includes:
	@echo installing ${STD_HEADERS}
.	for hdr src in ${STD}
		cmp -s ${src} ${DESTDIR}/${STDDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${STDDIR}/${hdr};
.	endfor
.	for hdr src in ${USTD}
		cmp -s ${src} ${DESTDIR}/${USTDDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${USTDDIR}/${hdr};
.	endfor
.	for hdr src in ${MACHO}
		cmp -s ${src} ${DESTDIR}/${MACHODIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${MACHODIR}/${hdr};
.	endfor


.include <bsd.lib.mk>
