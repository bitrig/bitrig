#	$OpenBSD: Makefile,v 1.121 2014/04/22 10:21:56 reyk Exp $

#
# For more information on building in tricky environments, please see
# the list of possible environment variables described in
# /usr/share/mk/bsd.README.
#
# Building recommendations:
#
# 1) If at all possible, put this source tree in /usr/src.  If /usr/src
# must be a symbolic link, set BSDSRCDIR in the environment to point to
# the real location.
#
# 2) It is also recommended that you compile with objects outside the
# source tree. To do this, ensure /usr/obj exists or points to some
# area of disk of sufficient size.  Then do "cd /usr/src; make obj".
# This will make a symbolic link called "obj" in each directory, as
# well as populate the /usr/obj properly with directories for the
# objects.
#
# 3) It is strongly recommended that you build and install a new kernel
# before rebuilding your system. Some of the new programs may use new
# functionality or depend on API changes that your old kernel doesn't have.
#
# 4) If you are reasonably sure that things will compile OK, use the
# "make build" target supplied here. Good luck.
#
# 5) If you want to setup a cross-build environment, there is a "cross-gcc"
# target available which upon completion of: 
#	make -f Makefile.cross TARGET=<target> cross-gcc"
# (where <target> is one of the names in the /sys/arch directory) will produce
# a set of compilation tools along with the includes in the /usr/cross/<target>
# directory. The "cross-distrib" target will build cross-tools as well as
# binaries for a given <target>.
#

.include <bsd.own.mk>	# for NOMAN, if it's there.

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

SUBDIR+= sys

.if   make(clean) || make(cleandir) || make(obj)
SUBDIR+= distrib regress
.endif

regression-tests:
	@echo Running regression tests...
	@cd ${.CURDIR}/regress && ${MAKE} depend && exec ${MAKE} regress

includes:
	cd ${.CURDIR}/include && ${MAKE} prereq && exec ${SUDO} ${MAKE} includes

beforeinstall:
	cd ${.CURDIR}/etc && exec ${MAKE} DESTDIR=${DESTDIR} distrib-dirs
	cd ${.CURDIR}/etc && exec ${MAKE} DESTDIR=${DESTDIR} install-mtree
	cd ${.CURDIR}/include && exec ${MAKE} includes

afterinstall:
.ifndef NOMAN
	cd ${.CURDIR}/share/man && exec ${MAKE} makedb
.endif

.ifdef DESTDIR
build buildworld:
	@echo cannot build with DESTDIR set
	@false
.else
mk_files:
	cd ${.CURDIR}/share/mk && exec ${SUDO} ${MAKE} install

build: mk_files
.ifdef GLOBAL_AUTOCONF_CACHE
	cp /dev/null ${GLOBAL_AUTOCONF_CACHE}
.endif
	cd ${.CURDIR}/usr.bin/clang && ${SUDO} NOMAN=1 ${MAKE} snapstrap && \
	    exec ${SUDO} ${MAKE} snapclean
	cd ${.CURDIR}/include && ${MAKE} prereq && exec ${SUDO} ${MAKE} includes
	${SUDO} ${MAKE} cleandir
	cd ${.CURDIR}/libexec/ld.so && \
	    SKIPDIR="ldd ldconfig" ${MAKE} depend && \
	    SKIPDIR="ldd ldconfig" ${MAKE} && \
	    ${SUDO} SKIPDIR="ldd ldconfig" NOMAN=1 ${MAKE} install
	cd ${.CURDIR}/lib/csu && ${MAKE} depend && ${MAKE} && \
	    NOMAN=1 exec ${SUDO} ${MAKE} install
	cd ${.CURDIR}/lib/libc && ${MAKE} depend && ${MAKE} && \
	    NOMAN=1 exec ${SUDO} ${MAKE} install
	cd ${.CURDIR}/lib && ${MAKE} depend && ${MAKE} && \
	    NOMAN=1 exec ${SUDO} ${MAKE} install
	${MAKE} depend && ${MAKE} && exec ${SUDO} ${MAKE} install

.ORDER: mk_files pre_world build
buildworld: mk_files pre_world build

pre_world:
	${SUDO} rm -rf /usr/obj/*
	${MAKE} obj >/dev/null
	cd ${.CURDIR}/etc && ${SUDO} DESTDIR=/ ${MAKE} distrib-dirs

.endif

CROSS_TARGETS=cross-env cross-dirs cross-obj cross-includes cross-binutils \
	cross-gcc cross-tools cross-lib cross-bin cross-etc-root-var \
	cross-distrib cross-depend cross-clean cross-cleandir

.if !defined(TARGET)
${CROSS_TARGETS}:
	@echo "TARGET must be set for $@"; exit 1
.else
. include "Makefile.cross"
.endif # defined(TARGET)

.PHONY: ${CROSS_TARGETS} \
	build regression-tests includes beforeinstall afterinstall \
	all depend

.if exists(/usr/snap)
SNAPDIR?=/usr/snap
.endif
.if !defined(SNAPDIR)
snapinfo snap do_snap do_snap_rel:
	@echo "SNAPDIR must be defined or create the directory /usr/snap"
	@exit 1
.else
.NOTPARALLEL:
ARCH!= uname -m
SNAPARCHDIR=${SNAPDIR}/${ARCH}
.if defined(SNAPDIRSUFFIX)
SNAPROOTDIR=${SNAPARCHDIR}/root.${SNAPDIRSUFFIX}
SNAPRELDIR=${SNAPARCHDIR}/release.${SNAPDIRSUFFIX}
SNAPLOGFILE= echo ${SNAPARCHDIR}/buildlog.${SNAPDIRSUFFIX}
.else
SNAPROOTDIR=${SNAPARCHDIR}/root
.if defined(SNAPDATE)
SNAPRELDIR!= echo ${SNAPARCHDIR}/release.$$(date "+%y%m%d%H%M")
.else
SNAPRELDIR!= echo ${SNAPARCHDIR}/release
.endif
SNAPLOGFILE != echo ${SNAPARCHDIR}/buildlog.$$(date "+%y%m%d%H%M")
.endif
snapinfo:
	@echo rootdir = ${SNAPROOTDIR}
	@echo reldir = ${SNAPRELDIR}
	@echo logfile = ${SNAPLOGFILE}

snap:  
	mkdir -p ${SNAPARCHDIR}
	${MAKE} do_snap 2>&1 | tee ${SNAPLOGFILE}

do_snap:  buildworld do_snap_rel

do_snap_rel:
	date
	rm -rf ${SNAPROOTDIR}
	mkdir -p ${SNAPROOTDIR}
	mkdir -p ${SNAPRELDIR}
	cd ${.CURDIR}/etc && DESTDIR=${SNAPROOTDIR} RELEASEDIR=${SNAPRELDIR} ${MAKE} release
	cd ${.CURDIR}/distrib/sets && DESTDIR=${SNAPROOTDIR} sh checkflist
.endif

.include <bsd.subdir.mk>
