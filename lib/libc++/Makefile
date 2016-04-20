HDRDIR=		${.CURDIR}/../../contrib/libc++/include
SRCDIR=		${.CURDIR}/../../contrib/libc++/src
CXXINCLUDEDIR=	/usr/include/c++/v1

.PATH: ${SRCDIR}

LIB=		c++

SRCS+=		any.cpp \
		algorithm.cpp\
		bind.cpp\
		chrono.cpp\
		condition_variable.cpp\
		debug.cpp\
		exception.cpp\
		future.cpp\
		hash.cpp\
		ios.cpp\
		iostream.cpp\
		locale.cpp\
		memory.cpp\
		mutex.cpp\
		new.cpp\
		random.cpp\
		regex.cpp\
		shared_mutex.cpp\
		stdexcept.cpp\
		string.cpp\
		strstream.cpp\
		system_error.cpp\
		thread.cpp\
		typeinfo.cpp\
		utility.cpp\
		valarray.cpp

WARNS=		0
CFLAGS+=	-nostdlib -DLIBCXXABI -I${CXXINCLUDEDIR} -I${.CURDIR}/../../contrib/libc++/src/include -DLIBCXX_BUILDING_LIBCXXABI
CPPFLAGS+=	${CFLAGS}
.if empty(CXXFLAGS:M-std=*)
CXXFLAGS+=	-std=c++11
.endif

LDFLAGS+=	--verbose
INCSGROUPS=	STD EXT EXP LOCALE

STD_HEADERS=	__bit_reference\
		__config\
		__debug\
		__functional_03\
		__functional_base\
		__functional_base_03\
		__hash_table\
		__locale\
		__mutex_base\
		__refstring\
		__split_buffer\
		__sso_allocator\
		__std_stream\
		__tree\
		__tuple\
		__undef_min_max\
		algorithm\
		array\
		atomic\
		bitset\
		cassert\
		ccomplex\
		cctype\
		cerrno\
		cfenv\
		cfloat\
		chrono\
		cinttypes\
		ciso646\
		climits\
		clocale\
		cmath\
		codecvt\
		complex\
		cxx_complex.h\
		condition_variable\
		csetjmp\
		csignal\
		cstdarg\
		cstdbool\
		cstddef\
		cstdint\
		cstdio\
		cstdlib\
		cstring\
		ctgmath\
		ctime\
		cwchar\
		cwctype\
		deque\
		exception\
		forward_list\
		fstream\
		functional\
		future\
		initializer_list\
		iomanip\
		ios\
		iosfwd\
		iostream\
		istream\
		iterator\
		limits\
		list\
		locale\
		map\
		memory\
		mutex\
		new\
		numeric\
		ostream\
		queue\
		random\
		ratio\
		regex\
		scoped_allocator\
		set\
		shared_mutex\
		sstream\
		stack\
		stdexcept\
		streambuf\
		string\
		strstream\
		system_error\
		cxx_tgmath.h\
		thread\
		tuple\
		type_traits\
		typeindex\
		typeinfo\
		unordered_map\
		unordered_set\
		utility\
		valarray\
		vector \
		__nullptr \
		__undef___deallocate \
		cxx_ctype.h \
		cxx_errno.h \
		cxx_float.h \
		cxx_inttypes.h \
		cxx_math.h \
		cxx_setjmp.h \
		cxx_stddef.h \
		cxx_stdio.h \
		cxx_stdlib.h \
		cxx_wchar.h \
		cxx_wctype.h

.for hdr in ${STD_HEADERS}
STD+=		${hdr} ${HDRDIR}/${hdr}
.endfor
STDDIR=		${CXXINCLUDEDIR}

EXT_HEADERS=	__hash\
		hash_map\
		hash_set

.for hdr in ${EXT_HEADERS}
EXT+=		${hdr} ${HDRDIR}/ext/${hdr}
.endfor
EXTDIR=		${CXXINCLUDEDIR}/ext

EXP_HEADERS=	__config \
		algorithm \
		any \
		chrono \
		functional \
		ratio \
		system_error \
		tuple

.for hdr in ${EXP_HEADERS}
EXP+=		${hdr} ${HDRDIR}/experimental/${hdr}
.endfor
EXPDIR=		${CXXINCLUDEDIR}/experimental

LOCALE_HEADERS=	xlocale.h

.for hdr in ${LOCALE_HEADERS}
LOCALE+=		${hdr} ${HDRDIR}/support/musl/${hdr}
.endfor
LOCALEDIR=		${CXXINCLUDEDIR}/support/musl

includes:
	@echo installing ${STD_HEADERS}
.	for hdr src in ${STD}
		cmp -s ${src} ${DESTDIR}/${STDDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${STDDIR}/${hdr};
.	endfor
.	for hdr src in ${EXT}
		cmp -s ${src} ${DESTDIR}/${EXTDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${EXTDIR}/${hdr};
.	endfor
.	for hdr src in ${EXP}
		cmp -s ${src} ${DESTDIR}/${EXPDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${EXPDIR}/${hdr};
.	endfor
.	for hdr src in ${LOCALE}
		cmp -s ${src} ${DESTDIR}/${LOCALEDIR}/${hdr} || \
		${INSTALL} ${INSTALL_COPY} -m 444 ${src} ${DESTDIR}/${LOCALEDIR}/${hdr};
.	endfor

.include <bsd.lib.mk>
