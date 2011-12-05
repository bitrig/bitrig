# Autoconf configuration file for groff.
# Copyright (C) 1989-1995, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
#               2009
# Free Software Foundation, Inc.
#
# This file is part of groff.
#
# groff is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# groff is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Process this file with autoconf to produce a configure script.

dnl Use `VERSION' and `REVISION' files to get version.
m4_define([GROFF_VERSION_DATA], m4_include([VERSION]))
m4_define([GROFF_REVISION_DATA], m4_include([REVISION]))

dnl Strip off newline characters.
m4_define([GROFF_VERSION],
          m4_substr(GROFF_VERSION_DATA,
                    [0], m4_decr(m4_len(GROFF_VERSION_DATA))))
m4_define([GROFF_REVISION],
          m4_substr(GROFF_REVISION_DATA,
                    [0], m4_decr(m4_len(GROFF_REVISION_DATA))))

dnl Don't use revision if it is zero.
m4_define([GROFF_VERSION_STRING],
          m4_if(m4_eval(GROFF_REVISION[ > 0]),
                [1],
                GROFF_VERSION[.]GROFF_REVISION,
                GROFF_VERSION))

AC_INIT([groff],
        GROFF_VERSION_STRING,
        [bug-groff@gnu.org],
        [groff-]GROFF_VERSION_STRING)
AC_PREREQ([2.62])

AC_CONFIG_HEADERS([src/include/config.h:src/include/config.hin])
AC_CONFIG_SRCDIR([src/roff/groff/groff.cpp])

AC_USE_SYSTEM_EXTENSIONS

# checks for programs
AC_PROG_CC
AC_PROG_CXX
GROFF_CXX_CHECK
GROFF_EBCDIC
GROFF_OS390
GROFF_X11
GROFF_APPRESDIR_OPTION
GROFF_APPRESDIR_DEFAULT
GROFF_PRINT
AC_PROG_EGREP
AC_PATH_PROG([PERLPATH], [perl], [/usr/bin/perl])
GROFF_PROG_YACC
GROFF_MAKEINFO
AC_PROG_RANLIB
GROFF_INSTALL_SH
GROFF_INSTALL_INFO
AC_PROG_INSTALL
AC_PROG_LN_S

# use a dummy substitution if no csh hack is necessary to avoid errors
# with non-GNU sed programs
GROFF_CSH_HACK([SH_SCRIPT_SED_CMD='1s/.*/:/'], [SH_SCRIPT_SED_CMD='1s/a/a/'])
AC_SUBST([SH_SCRIPT_SED_CMD])

# checks for headers
AC_CHECK_HEADERS([stddef.h stdlib.h unistd.h dirent.h limits.h sys/dir.h \
                  string.h strings.h math.h sys/time.h direct.h process.h])
GROFF_ISC_SYSV3
GROFF_POSIX

# checks for header stuff
GROFF_SRAND
GROFF_NEED_DECLARATION([gettimeofday])
GROFF_NEED_DECLARATION([hypot])
GROFF_NEED_DECLARATION([popen])
GROFF_NEED_DECLARATION([pclose])
GROFF_NEED_DECLARATION([putenv])
GROFF_NEED_DECLARATION([rand])
GROFF_NEED_DECLARATION([snprintf])
GROFF_NEED_DECLARATION([srand])
GROFF_NEED_DECLARATION([strcasecmp])
GROFF_NEED_DECLARATION([strncasecmp])
GROFF_NEED_DECLARATION([vfprintf])
GROFF_NEED_DECLARATION([vsnprintf])
GROFF_SYS_NERR
GROFF_SYS_ERRLIST
GROFF_OSFCN_H
GROFF_LIMITS_H
GROFF_INTTYPES_H

# checks for typedefs
GROFF_UNSIGNED_LONG_LONG
GROFF_UINTMAX_T
GROFF_TIME_T
GROFF_TYPE_SIGNAL
GROFF_STRUCT_EXCEPTION

# check endianness
AC_C_BIGENDIAN

# checks for libraries
GROFF_LIBC
GROFF_LIBM
AM_ICONV

# we need LIBEXT
AC_SUBST([LIBEXT],[$acl_libext])

# checks for functions
AC_FUNC_MMAP
saved_libs="$LIBS"
LIBS="$LIBS $LIBC $LIBM"
AC_REPLACE_FUNCS([fmod getcwd putenv snprintf strcasecmp \
                  strerror strncasecmp strtol])
# vsnprintf is in the same source file as snprintf
AC_CHECK_FUNCS([vsnprintf], [], [AC_LIBOBJ([snprintf])])
LIBS="$saved_libs"
AC_CHECK_FUNCS([gettimeofday isatty kill rename setlocale strsep])
GROFF_MKSTEMP
AC_CHECK_DECLS([sys_siglist, getc_unlocked])
AM_LANGINFO_CODESET

# checks for compiler characteristics
GROFF_ARRAY_DELETE
GROFF_TRADITIONAL_CPP

# checks for operating system services
GROFF_WCOREFLAG

# other random stuff
GROFF_BROKEN_SPOOLER_FLAGS
GROFF_PAGE
GROFF_G
GROFF_TMAC
GROFF_TARGET_PATH_SEPARATOR
GROFF_HTML_PROGRAMS
GROFF_PDFDOC_PROGRAMS
GROFF_PNMTOPS_NOSETPAGE
gl_GLIBC21

AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_CONFIG_FILES([Makefile doc/Makefile src/utils/xtotroff/Makefile])
AC_CONFIG_FILES([contrib/gdiffmk/tests/runtests],
                [chmod +x contrib/gdiffmk/tests/runtests])
AC_CONFIG_FILES([test-groff], [chmod +x test-groff])
AC_OUTPUT

GROFF_APPRESDIR_CHECK
