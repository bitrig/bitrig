#	$OpenBSD: bsd.own.mk,v 1.172 2015/10/26 10:43:42 bluhm Exp $
#	$NetBSD: bsd.own.mk,v 1.24 1996/04/13 02:08:09 thorpej Exp $

# Host-specific overrides
.if defined(MAKECONF) && exists(${MAKECONF})
.include "${MAKECONF}"
.elif exists(/etc/mk.conf)
.include "/etc/mk.conf"
.endif

# Set `WARNINGS' to `yes' to add appropriate warnings to each compilation
WARNINGS?=	no
# Set `SKEY' to `yes' to build with support for S/key authentication.
SKEY?=		yes
# Set `YP' to `yes' to build with support for NIS/YP.
YP?=		yes
# Set `DEBUGLIBS' to `yes' to build libraries with debugging symbols
DEBUGLIBS?=	no

GCC3_ARCH=m88k vax

# arm needs binutils-2.17, which still lacks W^X support
PIE_ARCH=amd64
#PIE_ARCH=alpha hppa i386 mips64 mips64el sh sparc64

.for _arch in ${MACHINE_ARCH}
.if !empty(PIE_ARCH:M${_arch})
NOPIE_FLAGS?=-fno-pie
NOPIE_LDFLAGS?=-nopie
PIE_DEFAULT?=${DEFAULT_PIE_DEF}
.else
NOPIE_FLAGS?=
PIE_DEFAULT?=
.endif
.endfor

.if ${COMPILER_VERSION} == "gcc4"
VISIBILITY_HIDDEN?=-fvisibility=hidden
.endif

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=	/usr/src
BSDOBJDIR?=	/usr/obj

BINGRP?=	bin
BINOWN?=	root
BINMODE?=	555
NONBINMODE?=	444
DIRMODE?=	755

SHAREDIR?=	/usr/share
SHAREGRP?=	bin
SHAREOWN?=	root
SHAREMODE?=	${NONBINMODE}

MANDIR?=	/usr/share/man/man
MANGRP?=	bin
MANOWN?=	root
MANMODE?=	${NONBINMODE}

LIBDIR?=	/usr/lib
LIBGRP?=	${BINGRP}
LIBOWN?=	${BINOWN}
LIBMODE?=	${NONBINMODE}

DOCDIR?=	/usr/share/doc
DOCGRP?=	bin
DOCOWN?=	root
DOCMODE?=	${NONBINMODE}

LKMDIR?=	/usr/lkm
LKMGRP?=	${BINGRP}
LKMOWN?=	${BINOWN}
LKMMODE?=	${NONBINMODE}

LOCALEDIR?=	/usr/share/locale
LOCALEGRP?=	wheel
LOCALEOWN?=	root
LOCALEMODE?=	${NONBINMODE}

.if !defined(CDIAGFLAGS)
CDIAGFLAGS=	-Wall -Wpointer-arith -Wuninitialized -Wstrict-prototypes
CDIAGFLAGS+=	-Wmissing-prototypes -Wunused -Wsign-compare
CDIAGFLAGS+=	-Wshadow
.  if ${COMPILER_VERSION} == "gcc4"
CDIAGFLAGS+=	-Wdeclaration-after-statement
.  endif
.endif

# Shared files for system gnu configure, not used yet
GNUSYSTEM_AUX_DIR?=${BSDSRCDIR}/share/gnu

INSTALL_COPY?=	-c
.ifndef DEBUG
INSTALL_STRIP?=	-s
.endif

# This may be changed for _single filesystem_ configurations (such as
# routers and other embedded systems); normal systems should leave it alone!
STATIC?=	-static

# Workaround for aarch64 attempting to link non-pie static binaries against
# rcrt0.o
STATIC+=-nopie

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks

# don't try to generate PIC versions of libraries on machines
# which don't support PIC.
.if ${MACHINE_ARCH} == "m88k" || ${MACHINE_ARCH} == "aarch64"
NOPIC=
.endif

# pic relocation flags.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64")
PICFLAG?=-fPIC
.else
PICFLAG?=-fpic
. if ${MACHINE_ARCH} == "m68k"
# Function CSE makes gas -k not recognize external function calls as lazily
# resolvable symbols, thus sometimes making ld.so report undefined symbol
# errors on symbols found in shared library members that would never be
# called.  Ask niklas@openbsd.org for details.
PICFLAG+=-fno-function-cse
. endif
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
ASPICFLAG=-KPIC
.endif

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
# big PIE
DEFAULT_PIE_DEF=-DPIE_DEFAULT=2
.else
# small pie
DEFAULT_PIE_DEF=-DPIE_DEFAULT=1
.endif

# don't try to generate PROFILED versions of libraries on machines
# which don't support profiling.
.if 0
NOPROFILE=
.endif

BSD_OWN_MK=Done

.PHONY: spell clean cleandir obj manpages print all \
	depend beforedepend afterdepend cleandepend subdirdepend \
	all cleanman includes \
	beforeinstall realinstall maninstall afterinstall install


# Define MK_INSTALLLIB for libraries which build using bsd.lib.mk but should
# not be installed, they link directly out of ${.OBJDIR}
MK_INSTALLLIB?=yes

MK_ENABLE_CLANG?=yes

# Control clang specific features
# MK_CLANG_EXTRAS controls if additional clang binaries besides the
# base compiler should be built
MK_CLANG_EXTRAS?=yes

# MK_CLANG_IS_CC controls if clang should be installed as /usr/bin/cc
MK_CLANG_IS_CC?=yes

# MK_SHARED_TOOLCHAIN forces compiler toolchain to build static (if set to no).
MK_SHARED_TOOLCHAIN?=yes

MK_ARM_EABI?=yes
