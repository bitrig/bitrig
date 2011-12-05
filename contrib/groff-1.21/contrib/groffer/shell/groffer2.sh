#! /bin/sh

# groffer - display groff files

# Source file position: <groff-source>/contrib/groffer/shell/groffer2.sh
# Installed position: <prefix>/lib/groff/groffer/groffer2.sh

# This file should not be run independently.  It is called by
# `groffer.sh' in the source or by the installed `groffer' program.

# Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2009
# Free Software Foundation, Inc.
# Written by Bernd Warken

# Last update: 5 Jan 2009

# This file is part of `groffer', which is part of `groff'.

# `groff' is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# `groff' is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


########################################################################
#             Test of rudimentary shell functionality
########################################################################

# Zsh is not Bourne compatible without the following:
if test -n "$ZSH_VERSION"; then
  emulate sh
  NULLCMD=:
fi


########################################################################
# Test of `unset'
#
export _UNSET;
export _foo;
_foo=bar;
_res="$(unset _foo 2>&1)";
if unset _foo >${_NULL_DEV} 2>&1 && \
   test _"${_res}"_ = __ && test _"${_foo}"_ = __
then
  _UNSET='unset';
  eval "${_UNSET}" _foo;
  eval "${_UNSET}" _res;
else
  _UNSET=':';
fi;


########################################################################
# Test of `test'.
#
if test a = a && test a != b && test -f "${_GROFFER_SH}"
then
  :;
else
  echo '"test" did not work.' >&2;
  exit "${_ERROR}";
fi;


########################################################################
# Test of `echo' and the `$()' construct.
#
if echo '' >${_NULL_DEV}
then
  :;
else
  echo '"echo" did not work.' >&2;
  exit "${_ERROR}";
fi;
if test _"$(t1="$(echo te)" &&
            t2="$(echo '')" &&
            t3="$(echo 'st')" &&
            echo "${t1}${t2}${t3}")"_ \
     != _test_
then
  echo 'The "$()" construct did not work' >&2;
  exit "${_ERROR}";
fi;


########################################################################
# Test of sed program; test in groffer.sh is not valid here.
#
if test _"$(echo xTesTx \
           | sed -n 's/^.\([Tt]e*x*sTT*\).*$/\1/p' \
           | sed 's|T|t|g')"_ != _test_
then
  echo 'The sed program did not work.' >&2;
  exit "${_ERROR}";
fi;


########################################################################
# Test of `cat'.
#
if test _"$(echo test | cat)"_ != _test_
then
  error 'Test of "cat" command failed.';
fi;


########################################################################
# Test of function definitions.
#
_t_e_s_t_f_u_n_c_()
{
  return 0;
}

_test_func()
{
  echo test;
}

if _t_e_s_t_f_u_n_c_ 2>${_NULL_DEV} && \
   test _"$(_test_func 2>${_NULL_DEV})"_ = _test_
then
  :;
else
  echo 'Shell '"${_SHELL}"' does not support function definitions.' >&2;
  exit "${_ERROR}";
fi;


########################################################################
# landmark (<text>)
#
# Print <text> to standard error as a debugging aid.
#
# Globals: $_DEBUG_LM
#
landmark()
{
  if test _"${_DEBUG_LM}"_ = _yes_
  then
    echo "LM: $*" >&2;
  fi;
} # landmark()


########################################################################
# test for compression.
#
export _HAS_COMPRESSION;
export _HAS_BZIP;
if test _"$(echo 'test' | gzip -c -d -f - 2>${_NULL_DEV})"_ = _test_
then
  _HAS_COMPRESSION='yes';
  if echo 'test' | bzip2 -c 2>${_NULL_DEV} | bzip2 -t 2>${_NULL_DEV} \
     && test _"$(echo 'test' | bzip2 -c 2>${_NULL_DEV} \
                             | bzip2 -d -c 2>${_NULL_DEV})"_ \
             = _test_
  then
    _HAS_BZIP='yes';
  else
    _HAS_BZIP='no';
  fi;
else
  _HAS_COMPRESSION='no';
  _HAS_BZIP='no';
fi;


########################################################################
#                    debug - diagnostic messages
########################################################################

export _DEBUG_FUNC_CHECK;
if test _"${_BEFORE_MAKE}"_ = _yes_
then
  _DEBUG_FUNC_CHECK='yes';
else
  _DEBUG_FUNC_CHECK='no';
fi;
_DEBUG_FUNC_CHECK='no';	# disable function checking
#_DEBUG_FUNC_CHECK='yes';	# enable function checking

export _DEBUG_STACKS;
_DEBUG_STACKS='no';		# disable stack output in each function
#_DEBUG_STACKS='yes';		# enable stack output in each function

export _DEBUG_USER_WITH_STACK;
_DEBUG_USER_WITH_STACK='no';	# disable stack dump in error_user()
#_DEBUG_USER_WITH_STACK='yes';	# enable stack dump in error_user()

export _DEBUG_LM;
_DEBUG_LM='no';			# disable landmark messages
#_DEBUG_LM='yes';		# enable landmark messages

export _DEBUG_GROG;
_DEBUG_GROG='no';		# disable grog output
#_DEBUG_GROG='yes';		# enable grog output

export _DEBUG_KEEP_FILES;
_DEBUG_KEEP_FILES='no'		# disable file keeping in temporary dir
#_DEBUG_KEEP_FILES='yes'	# enable file keeping in temporary dir

export _DEBUG_PRINT_PARAMS;
_DEBUG_PRINT_PARAMS='no';	# disable printing of all parameters
#_DEBUG_PRINT_PARAMS='yes';	# enable printing of all parameters

export _DEBUG_PRINT_SHELL;
_DEBUG_PRINT_SHELL='no';	# disable printing of the shell name
#_DEBUG_PRINT_SHELL='yes';	# enable printing of the shell name

export _DEBUG_PRINT_TMPDIR;
_DEBUG_PRINT_TMPDIR='no';	# disable printing of the temporary dir
#_DEBUG_PRINT_TMPDIR='yes';	# enable printing of the temporary dir

export _DEBUG_PRINT_FILENAMES;
_DEBUG_PRINT_FILENAMES='no';	# disable printing of the found file names
#_DEBUG_PRINT_FILENAMES='yes';	# enable printing of the found file names

# determine all --debug* options
case " $*" in
*\ --deb*|*\ --d*-*)
  # --debug-* options
  d=' --debug-all --debug-filenames --debug-func --debug-grog '\
'--debug-not-func --debug-keep --debug-lm --debug-params '\
'--debug-shell --debug-stacks --debug-tmpdir --debug-user ';
  # non-debug options with scheme --d*-*
  n=' --do-nothing --default-modes --dvi-viewer --dvi-viewer-tty ';
  for i
  do
    case "$i" in
    --deb|--debu|--debug)
      _DEBUG_FUNC_CHECK='yes';
      # _DEBUG_STACKS='yes';
      _DEBUG_USER_WITH_STACK='yes';
      # _DEBUG_LM='yes';
      _DEBUG_GROG='yes';
      _DEBUG_KEEP_FILES='yes';
      _DEBUG_PRINT_PARAMS='yes';
      _DEBUG_PRINT_SHELL='yes';
      _DEBUG_PRINT_TMPDIR='yes';
      _DEBUG_PRINT_FILENAMES='yes';
      continue;
      ;;
    --d*-*)
      # before `-'
      b="$(echo x"$i" | sed 's/^x--\([^-]*\)-.*$/\1/')";
      # after `-'
      a="$(echo x"$i" | sed 's/^x--[^-]*-\(.*\)$/\1/')";
      ;;
    *)
      continue;
      ;;
    esac;
    case "$n" in
    *\ --$b*-$a*)
      continue;
      ;;
    esac;
    case "$d" in
    *\ --$b*-$a*)
      case "$a" in
      f|s)			# double --debug-* options
        continue;
        ;;
      esac;
      # extract whole word of double abbreviation
      s="$(cat <<EOF | sed -n 's/^.* \(--'"$b"'[^ -]*-'"$a"'[^ ]*\) .*/\1/p'
$d
EOF
)"
      case "$s" in
      '') continue; ;;
      --debug-all)
        _DEBUG_FUNC_CHECK='yes';
        _DEBUG_STACKS='yes';
        _DEBUG_USER_WITH_STACK='yes';
        _DEBUG_GROG='yes';
        _DEBUG_LM='yes';
        _DEBUG_KEEP_FILES='yes';
        _DEBUG_PRINT_PARAMS='yes';
        _DEBUG_PRINT_SHELL='yes';
        _DEBUG_PRINT_TMPDIR='yes';
        _DEBUG_PRINT_FILENAMES='yes';
        _DEBUG_PRINT_FILENAMES='yes';
        ;;
      --debug-filenames)
        _DEBUG_PRINT_FILENAMES='yes';
        ;;
      --debug-func)
        _DEBUG_FUNC_CHECK='yes';
	;;
      --debug-not-func)
        _DEBUG_FUNC_CHECK='no';
        _DEBUG_STACKS='no';
        _DEBUG_USER_WITH_STACK='no';
	;;
      --debug-grog)
        _DEBUG_GROG='yes';
	;;
      --debug-keep)
        _DEBUG_PRINT_TMPDIR='yes';
        _DEBUG_KEEP_FILES='yes';
        ;;
      --debug-lm)
        _DEBUG_LM='yes';
        ;;
      --debug-params)
        _DEBUG_PRINT_PARAMS='yes';
        ;;
      --debug-shell)
        _DEBUG_PRINT_SHELL='yes';
        ;;
      --debug-stacks)
        _DEBUG_STACKS='yes';
        ;;
      --debug-tmpdir)
        _DEBUG_PRINT_TMPDIR='yes';
        ;;
      --debug-user)
        _DEBUG_USER_WITH_STACK='yes';
        ;;
      esac;
      ;;
    esac;
  done
  ;;
esac;

if test _"${_DEBUG_STACKS}"_ = _yes_ || \
   test _"${_DEBUG_USER_WITH_STACK}"_ = _yes_
then
  _DEBUG_FUNC_CHECK='yes';
fi

if test _"${_DEBUG_PRINT_PARAMS}"_ = _yes_
then
  echo "parameters: $@" >&2;
fi;

if test _"${_DEBUG_PRINT_SHELL}"_ = _yes_
then
  if test _"${_SHELL}"_ = __
  then
    if test _"${POSIXLY_CORRECT}"_ = _y_
    then
      echo 'shell: bash as /bin/sh (none specified)' >&2;
    else
      echo 'shell: /bin/sh (none specified)' >&2;
    fi;
  else
    echo "shell: ${_SHELL}" >&2;
  fi;
fi;


########################################################################
#                       Environment Variables
########################################################################

landmark "1: environment variables";

# Environment variables that exist only for this file start with an
# underscore letter.  Global variables to this file are written in
# upper case letters, e.g. $_GLOBAL_VARIABLE; temporary variables
# start with an underline and use only lower case letters and
# underlines, e.g.  $_local_variable.

#   [A-Z]*     system variables,      e.g. $MANPATH
#   _[A-Z_]*   global file variables, e.g. $_MAN_PATH
#   _[a-z_]*   temporary variables,   e.g. $_manpath

# Due to incompatibilities of the `ash' shell, the name of loop
# variables in `for' must be a single character.
#   [a-z]      local loop variables,   e.g. $i

# In functions, other writings are used for variables.  As some shells
# do not support the `local' command a unique prefix in lower case is
# constructed for each function, most are the abbreviation of the
# function name.  All variable names start with this prefix.
#   ${prefix}_[a-z_]*	variable name in a function, e.g. $msm_modes


########################################################################
# read-only variables (global to this file)
########################################################################

# function return values; `0' means ok; other values are error codes
export _BAD;
export _GOOD;
export _NO;
export _OK;
export _YES;

_GOOD='0';			# return ok
_BAD='1';			# return negatively, error code `1'
# $_ERROR was already defined as `7' in groffer.sh.

_NO="${_BAD}";
_YES="${_GOOD}";
_OK="${_GOOD}";

# quasi-functions, call with `eval', e.g `eval "${return_ok}"'
export return_ok;
export return_good;
export return_bad;
export return_yes;
export return_no;
export return_error;
export return_var;
return_ok="func_pop; return ${_OK}";
return_good="func_pop; return ${_GOOD}";
return_bad="func_pop; return ${_BAD}";
return_yes="func_pop; return ${_YES}";
return_no="func_pop; return ${_NO}";
return_error="func_pop; return ${_ERROR}";
return_var="func_pop; return";	# add number, e.g. `eval "${return_var} $n'


export _DEFAULT_MODES;
_DEFAULT_MODES="'pdf' 'html' 'ps' 'x' 'dvi' 'tty'";
export _DEFAULT_RESOLUTION;
_DEFAULT_RESOLUTION='75';

export _DEFAULT_TTY_DEVICE;
_DEFAULT_TTY_DEVICE='latin1';

# _VIEWER_* viewer programs for different modes constructed as lists
export _VIEWER_DVI_TTY;		# viewer program for dvi mode in tty
export _VIEWER_DVI_X;		# viewer program for dvi mode in X
export _VIEWER_HTML_TTY;	# viewer program for html mode in tty
export _VIEWER_HTML_X;		# viewer program for html mode in X
export _VIEWER_PDF_TTY;		# viewer program for pdf mode in tty
export _VIEWER_PDF_X;		# viewer program for pdf mode in X
export _VIEWER_PS_TTY;		# viewer program for ps mode in tty
export _VIEWER_PS_X;		# viewer program for ps mode in X
export _VIEWER_TTY_TTY;		# viewer program for X/x mode in tty
export _VIEWER_TTY_X;		# viewer program for X/x mode in X
export _VIEWER_X_TTY;		# viewer program for X/x mode in tty
export _VIEWER_X_X;		# viewer program for X/x mode in X
_VIEWER_DVI_TTY="";
_VIEWER_DVI_X="'kdvi' 'xdvi' 'dvilx'";
_VIEWER_HTML_TTY="'lynx' 'w3m'";
_VIEWER_HTML_X="'konqueror' 'epiphany' 'mozilla-firefox' 'firefox' 'mozilla' \
'netscape' 'galeon' 'opera' 'amaya' 'arena' 'mosaic'";
_VIEWER_PDF_TTY="";
_VIEWER_PDF_X="'kpdf' 'acroread' 'evince' 'xpdf -z 150' 'gpdf' \
'kghostview --scale 1.45' 'ggv'";
_VIEWER_PS_TTY="";
_VIEWER_PS_X="'kpdf' 'kghostview --scale 1.45' 'evince' 'ggv' 'gv' \
'ghostview' 'gs_x11,gs'";
_VIEWER_TTY_TTY="'less -r -R' 'more' 'pager'";
_VIEWER_TTY_X="'xless'";
_VIEWER_X_TTY="";
_VIEWER_X_X="'gxditview' 'xditview'";

# Search automatically in standard sections `1' to `8', and in the
# traditional sections `9', `n', and `o'.  On many systems, there
# exist even more sections, mostly containing a set of man pages
# special to a specific program package.  These aren't searched for
# automatically, but must be specified on the command line.
export _MAN_AUTO_SEC_LIST;
_MAN_AUTO_SEC_LIST="'1' '2' '3' '4' '5' '6' '7' '8' '9' 'n' 'o'";
export _MAN_AUTO_SEC_CHARS;
_MAN_AUTO_SEC_CHARS='[123456789no]';

export _SPACE_SED;
_SPACE_SED='['"${_SP}${_TAB}"']';

export _SPACE_CASE;
_SPACE_CASE='[\'"${_SP}"'\'"${_TAB}"']';

export _PROCESS_ID;		# for shutting down the program
_PROCESS_ID="$$";

export _START_DIR;		# directory at start time of the script
_START_DIR="$(pwd)";


############ the command line options of the involved programs
#
# The naming scheme for the options environment names is
# $_OPTS_<prog>_<length>[_<argspec>]
#
# <prog>:    program name GROFFER, GROFF, or CMDLINE (for all
#            command line options)
# <length>:  LONG (long options) or SHORT (single character options)
# <argspec>: ARG for options with argument, NA for no argument;
#            without _<argspec> both the ones with and without arg.
#
# Each option that takes an argument must be specified with a
# trailing : (colon).

# exports
export _OPTS_GROFFER_SHORT_NA;
export _OPTS_GROFFER_SHORT_ARG;
export _OPTS_GROFFER_LONG_NA;
export _OPTS_GROFFER_LONG_ARG;
export _OPTS_GROFF_SHORT_NA;
export _OPTS_GROFF_SHORT_ARG;
export _OPTS_GROFF_LONG_NA;
export _OPTS_GROFF_LONG_ARG;
export _OPTS_X_SHORT_ARG;
export _OPTS_X_SHORT_NA;
export _OPTS_X_LONG_ARG;
export _OPTS_X_LONG_NA;
export _OPTS_MAN_SHORT_ARG;
export _OPTS_MAN_SHORT_NA;
export _OPTS_MAN_LONG_ARG;
export _OPTS_MAN_LONG_NA;
export _OPTS_MANOPT_SHORT_ARG;
export _OPTS_MANOPT_SHORT_NA;
export _OPTS_MANOPT_LONG_ARG;
export _OPTS_MANOPT_LONG_NA;
export _OPTS_CMDLINE_SHORT_NA;
export _OPTS_CMDLINE_SHORT_ARG;
export _OPTS_CMDLINE_LONG_NA;
export _OPTS_CMDLINE_LONG_ARG;

###### groffer native options

_OPTS_GROFFER_SHORT_NA="'h' 'Q' 'v' 'V' 'X' 'Z'";
_OPTS_GROFFER_SHORT_ARG="'T'";

_OPTS_GROFFER_LONG_NA="'auto' \
'apropos' 'apropos-data' 'apropos-devel' 'apropos-progs' \
'debug' 'debug-all' 'debug-filenames' \
'debug-func' 'debug-not-func' 'debug-grog' 'debug-keep' 'debug-lm' \
'debug-params' 'debug-shell' 'debug-stacks' 'debug-tmpdir' 'debug-user' \
'default' 'do-nothing' 'dvi' 'groff' 'help' 'intermediate-output' 'html' \
'man' 'no-location' 'no-man' 'no-special' 'pdf' 'ps' 'rv' 'source' \
'text' 'to-stdout' 'text-device' 'tty' 'tty-device' \
'version' 'whatis' 'www' 'x' 'X'";

_OPTS_GROFFER_LONG_ARG="\
'default-modes' 'device' 'dvi-viewer' 'dvi-viewer-tty' 'extension' 'fg' \
'fn' 'font' 'foreground' 'html-viewer' 'html-viewer-tty' 'mode' \
'pdf-viewer' 'pdf-viewer-tty' 'print' 'ps-viewer' 'ps-viewer-tty' 'shell' \
'title' 'tty-viewer' 'tty-viewer-tty' 'www-viewer' 'www-viewer-tty' \
'x-viewer' 'x-viewer-tty' 'X-viewer' 'X-viewer-tty'";

##### groffer options inhereted from groff

_OPTS_GROFF_SHORT_NA="'a' 'b' 'c' 'C' 'e' 'E' 'g' 'G' 'i' 'k' 'l' 'N' 'p' \
'R' 's' 'S' 't' 'U' 'z'";
_OPTS_GROFF_SHORT_ARG="'d' 'f' 'F' 'I' 'K' 'L' 'm' 'M' 'n' 'o' 'P' 'r' \
'w' 'W'";
_OPTS_GROFF_LONG_NA="";
_OPTS_GROFF_LONG_ARG="";

##### groffer options inhereted from the X Window toolkit

_OPTS_X_SHORT_NA="";
_OPTS_X_SHORT_ARG="";

_OPTS_X_LONG_NA="'iconic' 'rv'";

_OPTS_X_LONG_ARG="'background' 'bd' 'bg' 'bordercolor' 'borderwidth' \
'bw' 'display' 'fg' 'fn' 'font' 'foreground' 'ft' 'geometry' \
'resolution' 'title' 'xrm'";

###### groffer options inherited from man

_OPTS_MAN_SHORT_NA="";
_OPTS_MAN_SHORT_ARG="";

_OPTS_MAN_LONG_NA="'all' 'ascii' 'catman' 'ditroff' \
'local-file' 'location' 'troff' 'update' 'where'";

_OPTS_MAN_LONG_ARG="'locale' 'manpath' \
'pager' 'preprocessor' 'prompt' 'sections' 'systems' 'troff-device'";

###### additional options for parsing $MANOPT only

_OPTS_MANOPT_SHORT_NA="'7' 'a' 'c' 'd' 'D' 'f' 'h' 'k' 'l' 't' 'u' \
'V' 'w' 'Z'";
_OPTS_MANOPT_SHORT_ARG="'e' 'L' 'm' 'M' 'p' 'P' 'r' 'S' 'T'";

_OPTS_MANOPT_LONG_NA="${_OPTS_MAN_LONG_NA} \
'apropos' 'debug' 'default' 'help' 'html' 'ignore-case' 'location-cat' \
'match-case' 'troff' 'update' 'version' 'whatis' 'where' 'where-cat'";

_OPTS_MANOPT_LONG_ARG="${_OPTS_MAN_LONG_NA} \
'config_file' 'encoding' 'extension' 'locale'";

###### collections of command line options

_OPTS_CMDLINE_SHORT_NA="${_OPTS_GROFFER_SHORT_NA} \
${_OPTS_GROFF_SHORT_NA} ${_OPTS_X_SHORT_NA} ${_OPTS_MAN_SHORT_NA}";
_OPTS_CMDLINE_SHORT_ARG="${_OPTS_GROFFER_SHORT_ARG} \
${_OPTS_GROFF_SHORT_ARG} ${_OPTS_X_SHORT_ARG} ${_OPTS_MAN_SHORT_ARG}";

_OPTS_CMDLINE_LONG_NA="${_OPTS_GROFFER_LONG_NA} \
${_OPTS_GROFF_LONG_NA} ${_OPTS_X_LONG_NA} ${_OPTS_MAN_LONG_NA}";
_OPTS_CMDLINE_LONG_ARG="${_OPTS_GROFFER_LONG_ARG} \
${_OPTS_GROFF_LONG_ARG} ${_OPTS_MAN_LONG_ARG} ${_OPTS_X_LONG_ARG}";


########################################################################
# read-write variables (global to this file)
########################################################################

export _ALL_PARAMS;		# All options and file name parameters
export _ADDOPTS_GROFF;		# Transp. options for groff (`eval').
export _APROPOS_PROG;		# Program to run apropos.
export _APROPOS_SECTIONS;	# Sections for different --apropos-*.
export _DISPLAY_MODE;		# Display mode.
export _DISPLAY_PROG;		# Viewer program to be used for display.
export _DISPLAY_ARGS;		# X resources for the viewer program.
export _FILE_NR;		# number for temporary `,file,*'
export _FILEARGS;		# Stores filespec parameters.
export _FILESPEC_ARG;		# Stores the actual filespec parameter.
export _FILESPEC_IS_MAN;	# filespec is for searching a man page
export _FUNC_STACK;		# Store debugging information.
export _MACRO_PACKAGES;		# groff's macro packages.
export _MACRO_PKG;		# Macro package for each found file.
export _NO_FILESPECS;		# Yes, if there are no filespec arguments.
export _OUTPUT_FILE_NAME;	# output generated, see main_set_res..()
export _REG_TITLE_LIST;		# Processed file names.
export _SOELIM_R;		# option -r for soelim
export _SPECIAL_FILESPEC;	# Filespec ran for apropos or whatis.
export _SPECIAL_SETUP;		# Test on setup for apropos or whatis.
export _VIEWER_BACKGROUND;	# viewer shall be run in the background or not
# _MAN_* finally used configuration of man searching
export _MAN_ALL;		# search all man pages per filespec
export _MAN_ENABLE;		# enable search for man pages
export _MAN_EXT;		# extension for man pages
export _MAN_FORCE;		# force file parameter to be man pages
export _MAN_IS_SETUP;		# setup man variables only once
export _MAN_LANG;		# language for man pages
export _MAN_LANG2;		# language for man pages
export _MAN_PATH;		# search path for man pages as a list
export _MAN_SEC;		# sections for man pages; sep. `:'
export _MAN_SEC_CHARS;		# sections for man pages as [] construct
export _MAN_SEC_LIST;		# sections for man pages as a list
export _MAN_SYS;		# system names for man pages as a list
# _MANOPT_* as parsed from $MANOPT
export _MANOPT_ALL;		# $MANOPT --all
export _MANOPT_EXTENSION;	# $MANOPT --extension
export _MANOPT_LANG;		# $MANOPT --locale
export _MANOPT_PATH;		# $MANOPT --manpath
export _MANOPT_PAGER;		# $MANOPT --pager
export _MANOPT_SEC;		# $MANOPT --sections
export _MANOPT_SYS;		# $MANOPT --systems
# variables for mode pdf
export _PDF_DID_NOT_WORK;
export _PDF_HAS_GS;
export _PDF_HAS_PS2PDF;
# _OPT_* as parsed from groffer command line
export _OPT_ALL;		# display all suitable man pages
export _OPT_APROPOS;		# call `apropos' program
export _OPT_BD;			# set border color in some modes
export _OPT_BG;			# set background color in some modes
export _OPT_BW;			# set border width in some modes
export _OPT_DEFAULT_MODES;	# `,'-list of modes when no mode given
export _OPT_DEVICE;		# device option
export _OPT_DO_NOTHING;		# do nothing in main_display()
export _OPT_DISPLAY;		# set X display
export _OPT_EXTENSION;		# set extension for man page search
export _OPT_FG;			# set foreground color in some modes
export _OPT_FN;			# set font in some modes
export _OPT_GEOMETRY;		# set size and position of viewer in X
export _OPT_ICONIC;		# -iconic option for X viewers
export _OPT_LANG;		# set language for man pages
export _OPT_MODE;		# values: X, tty, Q, Z, ""
export _OPT_MANPATH;		# manual setting of path for man-pages
export _OPT_PAGER;		# specify paging program for tty mode
export _OPT_RESOLUTION;		# set X resolution in dpi
export _OPT_RV;			# reverse fore- and background colors
export _OPT_SECTIONS;		# sections for man page search
export _OPT_STDOUT;		# print mode file to standard output
export _OPT_SYSTEMS;		# man pages of different OS's
export _OPT_TITLE;		# title for gxditview window
export _OPT_TEXT_DEVICE;	# set device for tty mode
export _OPT_V;			# groff option -V
export _OPT_VIEWER_DVI;		# viewer program for dvi mode
export _OPT_VIEWER_HTML;	# viewer program for html mode
export _OPT_VIEWER_PDF;		# viewer program for pdf mode
export _OPT_VIEWER_PS;		# viewer program for ps mode
export _OPT_VIEWER_X;		# viewer program for x mode
export _OPT_WHATIS;		# print the man description
export _OPT_XRM;		# specify X resource
export _OPT_Z;			# groff option -Z
# _TMP_* temporary directory and files
export _TMP_DIR;		# groffer directory for temporary files
export _TMP_CAT;		# stores concatenation of everything
export _TMP_MAN;		# stores find of man path
export _TMP_MANSPEC;		# filters man pages with filespec
export _TMP_STDIN;		# stores stdin, if any

# these variables are preset in section `Preset' after the rudim. test


########################################################################
# Preset and reset of read-write global variables
########################################################################

# For variables that can be reset by option `--default', see reset().

_FILE_NR=0;
_FILEARGS='';
_MACRO_PACKAGES="'-man' '-mdoc' '-me' '-mm' '-mom' '-ms'";
_SPECIAL_FILESPEC='no';
_SPECIAL_SETUP='no';

# _TMP_* temporary files
_TMP_DIR='';
_TMP_CAT='';
_TMP_MAN='';
_TMP_CONF='';
_TMP_STDIN='';

# variables for mode pdf
_PDF_DID_NOT_WORK='no';
_PDF_HAS_GS='no';
_PDF_HAS_PS2PDF='no';

# option -r for soelim
if : | soelim -r 2>${_NULL_DEV} >${_NULL_DEV}
then
  _SOELIM_R='-r';
else
  _SOELIM_R='';
fi;

########################################################################
# reset ()
#
# Reset the variables that can be affected by options to their default.
#
reset()
{
  if test "$#" -ne 0
  then
    error "reset() does not have arguments.";
  fi;

  _ADDOPTS_GROFF='';
  _APROPOS_PROG='';
  _APROPOS_SECTIONS='';
  _DISPLAY_ARGS='';
  _DISPLAY_MODE='';
  _DISPLAY_PROG='';
  _MACRO_PKG='';
  _NO_FILESPECS='';
  _REG_TITLE_LIST='';

  # _MAN_* finally used configuration of man searching
  _MAN_ALL='no';
  _MAN_ENABLE='yes';		# do search for man-pages
  _MAN_EXT='';
  _MAN_FORCE='no';		# first local file, then search man page
  _MAN_IS_SETUP='no';
  _MAN_LANG='';
  _MAN_LANG2='';
  _MAN_PATH='';
  _MAN_SEC='';
  _MAN_SEC_CHARS='';
  _MAN_SEC_LIST='';
  _MAN_SYS='';

  # _MANOPT_* as parsed from $MANOPT
  _MANOPT_ALL='no';
  _MANOPT_EXTENSION='';
  _MANOPT_LANG='';
  _MANOPT_PATH='';
  _MANOPT_PAGER='';
  _MANOPT_SEC='';
  _MANOPT_SYS='';

  # _OPT_* as parsed from groffer command line
  _OPT_ALL='no';
  _OPT_APROPOS='no';
  _OPT_BD='';
  _OPT_BG='';
  _OPT_BW='';
  _OPT_DEFAULT_MODES='';
  _OPT_DEVICE='';
  _OPT_DISPLAY='';
  _OPT_DO_NOTHING='no';
  _OPT_EXTENSION='';
  _OPT_FG='';
  _OPT_FN='';
  _OPT_GEOMETRY='';
  _OPT_ICONIC='no';
  _OPT_LANG='';
  _OPT_MODE='';
  _OPT_MANPATH='';
  _OPT_PAGER='';
  _OPT_RESOLUTION='';
  _OPT_RV='no';
  _OPT_SECTIONS='';
  _OPT_SYSTEMS='';
  _OPT_STDOUT='no';
  _OPT_TITLE='';
  _OPT_TEXT_DEVICE='';
  _OPT_V='no';
  _OPT_VIEWER_DVI='';
  _OPT_VIEWER_PDF='';
  _OPT_VIEWER_PS='';
  _OPT_VIEWER_HTML='';
  _OPT_VIEWER_X='';
  _OPT_WHATIS='no';
  _OPT_XRM='';
  _OPT_Z='no';
  _VIEWER_BACKGROUND='no';
}

reset;


########################################################################
#              Preliminary functions for error handling
########################################################################

landmark "2: preliminary functions";

# These functions do not have a func-check frame.  Basically they could be
# moved to the functions in alphabetical order.

##############
# echo1 (<text>*)
#
# Output to stdout with final line break.
#
# Arguments : arbitrary text including `-'.
#
echo1()
{
  cat <<EOF
$@
EOF
} # echo1()


##############
# echo2 (<text>*)
#
# Output to stderr with final line break.
#
# Arguments : arbitrary text including `-'.
#
echo2()
{
  cat >&2 <<EOF
$@
EOF
} # echo2()




##############
# clean_up ()
#
# Clean up at exit.
#
cu_already='no';
clean_up()
{
  cd "${_START_DIR}" >"${_NULL_DEV}" 2>&1;
  if test _${_DEBUG_KEEP_FILES}_ = _yes_
  then
    if test _"$cu_already"_ = _yes_
    then
      eval "${return_ok}";
    fi;
    cu_already=yes;
    echo2 "Kept temporary directory ${_TMP_DIR}."
  else
    if test _"${_TMP_DIR}"_ != __
    then
      if test -e "${_TMP_DIR}"
      then
        rm -f -r "${_TMP_DIR}" >${_NULL_DEV} 2>&1;
      fi;
    fi;
  fi;
  eval "${return_ok}";
} # clean_up()


#############
# diag (text>*)
#
# Output a diagnostic message to stderr.
#
diag()
{
  echo2 '>>>>>'"$*";
} # diag()


#############
# error (<text>*)
#
# Print an error message to standard error, print the function stack,
# exit with an error condition.  The argument should contain the name
# of the function from which it was called.  This is for system errors.
#
error()
{
  case "$#" in
    1) echo2 'groffer error: '"$1"; ;;
    *) echo2 'groffer error: wrong number of arguments in error().'; ;;
  esac;
  func_stack_dump;
  if test _"${_TMP_DIR}"_ != __ && test -d "${_TMP_DIR}"
  then
    : >"${_TMP_DIR}"/,error;
  fi;
  exit "${_ERROR}";
} # error()


#############
# error_user (<text>*)
#
# Print an error message to standard error; exit with an error condition.
# The error is supposed to be produced by the user.  So the funtion stack
# is omitted.
#
error_user()
{
  case "$#" in
    1)
      echo2 'groffer error: '"$1";
       ;;
    *)
      echo2 'groffer error: wrong number of arguments in error_user().';
      ;;
  esac;
  if test _"${_DEBUG_USER_WITH_STACK}"_ = _yes_
  then
    func_stack_dump;
  fi;
  if test _"${_TMP_DIR}"_ != __ && test -d "${_TMP_DIR}"
  then
    : >"${_TMP_DIR}"/,error;
  fi;
  exit "${_ERROR}";
} # error_user()



#############
# exit_test ()
#
# Test whether the former command ended with error().  Exit again.
#
# Globals: $_ERROR
#
exit_test()
{
  if test "$?" = "${_ERROR}"
  then
    exit ${_ERROR};
  fi;
  if test _"${_TMP_DIR}"_ != __ && test -f "${_TMP_DIR}"/,error
  then
    exit ${_ERROR};
  fi;
} # exit_test()


########################################################################
#       Definition of normal Functions in alphabetical order
########################################################################

landmark "3: functions";

########################################################################
# apropos_filespec ()
#
# Compose temporary file for filspec.
#
# Globals:  in: $_OPT_APROPOS, $_SPECIAL_SETUP, $_FILESPEC_ARG,
#               $_APROPOS_PROG, $_APROPOS_SECTIONS, $_OPT_SECTIONS
#          out: $_SPECIAL_FILESPEC
#
# Variable prefix: af
#
apropos_filespec()
{

  func_check apropos_filespec '=' 0 "$@";
  if obj _OPT_APROPOS is_yes
  then
    if obj _SPECIAL_SETUP is_not_yes
    then
      error 'apropos_filespec(): apropos_setup() must be run first.';
    fi;
    _SPECIAL_FILESPEC='yes';
    if obj _NO_FILESPECS is_yes
    then
      to_tmp_line '.SH no filespec';
      eval "${_APROPOS_PROG}" | sed 's/^/\\\&/' >>"${_TMP_CAT}";
      eval "${return_ok}";
    fi;
    eval to_tmp_line \
      "'.SH $(echo1 "${_FILESPEC_ARG}" | sed 's/[^\\]-/\\-/g')'";
    exit_test;
    if obj _APROPOS_SECTIONS is_empty
    then
      if obj _OPT_SECTIONS is_empty
      then
        s='^.*(..*).*$';
      else
        s='^.*(['"$(echo1 "${_OPT_SECTIONS}" | sed 's/://g')"']';
      fi;
    else
      s='^.*(['"${_APROPOS_SECTIONS}"']';
    fi;
### apropos_filespec()
    af_filespec="$(echo1 "${_FILESPEC_ARG}" | sed '
s,/,\\/,g
s/\./\\./g
')";
    eval "${_APROPOS_PROG}" "'${_FILESPEC_ARG}'" | \
      sed -n '
/^'"${af_filespec}"': /s/^\(.*\)$/\\\&\1/p
/'"$s"'/p
' | \
      sort |\
      sed '
s/^\(.*(..*).*\)  *-  *\(.*\)$/\.br\n\.TP 15\n\.BR \"\1\"\n\\\&\2/
' >>"${_TMP_CAT}";
    eval ${_UNSET} af_filespec;
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # apropos_filespec()


########################################################################
# apropos_setup ()
#
# Setup for the --apropos* options, just 2 global variables are set.
#
# Globals:  in: $_OPT_APROPOS
#          out: $_SPECIAL_SETUP, $_APROPOS_PROG
#
apropos_setup()
{
  func_check apropos_setup '=' 0 "$@";
  if obj _OPT_APROPOS is_yes
  then
    if is_prog apropos
    then
      _APROPOS_PROG='apropos';
    elif is_prog man
    then
      if man --apropos man >${_NULL_DEV} 2>${_NULL_DEV}
      then
        _APROPOS_PROG='man --apropos';
      elif man -k man >${_NULL_DEV} 2>${_NULL_DEV}
      then
        _APROPOS_PROG='man -k';
      fi;
    fi;
    if obj _APROPOS_PROG is_empty
    then
      error 'apropos_setup(): no apropos program available.';
    fi;
    to_tmp_line '.TH GROFFER APROPOS';
    _SPECIAL_SETUP='yes';
    if obj _OPT_TITLE is_empty
    then
      _OPT_TITLE='apropos';
    fi;
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # apropos_setup()


########################################################################
# base_name (<path>)
#
# Get the file name part of <path>, i.e. delete everything up to last
# `/' from the beginning of <path>.  Remove final slashes, too, to get
# a non-empty output.  The output is constructed according the shell
# program `basename'.
#
# Arguments : 1
# Output    : the file name part (without slashes)
#
# Variable prefix: bn
#
base_name()
{
  func_check base_name = 1 "$@";
  bn_name="$1";
  case "${bn_name}" in
    */)
      # delete all final slashes
      bn_name="$(echo1 "${bn_name}" | sed 's|//*$||')";
      exit_test;
      ;;
  esac;
  case "${bn_name}" in
    '')
      eval ${_UNSET} bn_name;
      eval "${return_bad}";
      ;;
    /)
      # this is like `basename' does
      echo1 '/';
      ;;
    */*)
      # delete everything before and including the last slash `/'.
      echo1 "${bn_name}" | sed 's|^.*//*\([^/]*\)$|\1|';
      ;;
    *)
      obj bn_name echo1;
      ;;
  esac;
  eval ${_UNSET} bn_name;
  eval "${return_ok}";
} # base_name()


########################################################################
# cat_z (<file>)
#
# Decompress if possible or just print <file> to standard output.
# gzip, bzip2, and .Z decompression is supported.
#
# Arguments: 1, a file name.
# Output: the content of <file>, possibly decompressed.
#
cat_z()
{
  func_check cat_z = 1 "$@";
  case "$1" in
  '')
    error 'cat_z(): empty file name.';
    ;;
  '-')
    error 'cat_z(): for standard input use save_stdin().';
    ;;
  esac;
  if is_file "$1"
  then
    :;
  else
    error 'cat_z(): argument $1 is not a file.';
  fi;
  if test -s "$1"
  then
    :;
  else
    eval "${return_ok}";
  fi;
  if obj _HAS_COMPRESSION is_yes
  then
    if obj _HAS_BZIP is_yes
    then
      # test whether being compressed with bz2
      if bzip2 -t "$1" 2>${_NULL_DEV}
      then
        bzip2 -c -d "$1" 2>${_NULL_DEV};
        eval "${return_ok}";
      fi;
    fi;
    # if not compressed gzip acts like `cat'
    gzip -c -d -f "$1" 2>${_NULL_DEV};
  else
    cat "$1";
  fi;
  eval "${return_ok}";
} # cat_z()


########################################################################
# clean_up ()
#
# Do the final cleaning up before exiting; used by the trap calls.
#
# defined above


########################################################################
# diag (<text>*)
#
# Print marked message to standard error; useful for debugging.
#
# defined above


########################################################################
landmark '4: dir_name()*';
########################################################################

#######################################################################
# dir_name (<name>)
#
# Get the directory name of <name>.  The output is constructed
# according to the shell program `dirname'.
#
# Arguments : 1
# Output    : the directory part of <name>
#
# Variable prefix: dn
#
dir_name()
{
  func_check dir_name = 1 "$@";
  obj_from_output dn_name dir_name_chop "$1";
  case "${dn_name}" in
  ''|.)
    echo1 '.';
    ;;
  /)
    echo1 '/';
    ;;
  */*)
    echo1 "$(echo1 "${dn_name}" | sed 's#/*[^/][^/]*$##')";
    ;;
  *)
    echo1 "${dn_name}";
    ;;
  esac;
  eval "${return_ok}";
} # dir_name()


#######################################################################
# dir_name_append (<dir> <name>)
#
# Append `name' to `dir' with clean handling of `/'.
#
# Arguments : 2
# Output    : the generated new directory name <dir>/<name>
#
dir_name_append()
{
  func_check dir_name_append = 2 "$@";
  if is_empty "$1"
  then
    echo1 "$2";
  elif is_empty "$2"
  then
    echo1 "$1";
  else
    dir_name_chop "$1"/"$2";
  fi;
  eval "${return_ok}";
} # dir_name_append()


########################################################################
# dir_name_chop (<name>)
#
# Remove unnecessary slashes from directory name.
#
# Argument: 1, a directory name.
# Output:   path without double, or trailing slashes.
#
# Variable prefix: dc
#
dir_name_chop()
{
  func_check dir_name_chop = 1 "$@";
  # replace all multiple slashes by a single slash `/'.
  dc_res="$(echo1 "$1" | sed 's|///*|/|g')";
  exit_test;
  case "${dc_res}" in
  ?*/)
    # remove trailing slash '/';
    echo1 "${dc_res}" | sed 's|/$||';
    ;;
  *)
    obj dc_res echo1
    ;;
  esac;
  eval ${_UNSET} dc_res;
  eval "${return_ok}";
} # dir_name_chop()


########################################################################
# do_nothing ()
#
# Dummy function that does nothing.
#
do_nothing()
{
  eval return "${_OK}";
} # do_nothing()


########################################################################
# echo1 (<text>*)
#
# Print to standard output with final line break.
#
# defined above


########################################################################
# echo2 (<text>*)
#
# Print to standard error with final line break.
#
# defined above



########################################################################
# error (<text>*)
#
# Print error message and exit with error code.
#
# defined above


########################################################################
# exit_test ()
#
# Test whether the former command ended with error().  Exit again.
#
# defined above


if test _"${_DEBUG_FUNC_CHECK}"_ = _yes_
then

  #############
  # func_check (<func_name> <rel_op> <nr_args> "$@")
  #
  # This is called at the first line of each function.  It checks the
  # number of arguments of function <func_name> and registers the
  # function call to _FUNC_STACK.
  #
  # Arguments: >=3
  #   <func_name>: name of the calling function.
  #   <rel_op>:    a relational operator: = != < > <= >=
  #   <nr_args>:   number of arguments to be checked against <operator>
  #   "$@":        the arguments of the calling function.
  #
  # Variable prefix: fc
  #
  func_check()
  {
    if test "$#" -lt 3
    then
      error 'func_check() needs at least 3 arguments.';
    fi;
    fc_fname="$1";
    case "$3" in
    1)
      fc_nargs="$3";
      fc_s='';
      ;;
    0|[2-9])
      fc_nargs="$3";
      fc_s='s';
      ;;
    *)
      error "func_check(): third argument must be a digit.";
      ;;
    esac;
### func_check()
    case "$2" in
    '='|'-eq')
      fc_op='-eq';
      fc_comp='exactly';
      ;;
    '>='|'-ge')
      fc_op='-ge';
      fc_comp='at least';
      ;;
    '<='|'-le')
      fc_op='-le';
      fc_comp='at most';
      ;;
    '<'|'-lt')
      fc_op='-lt';
      fc_comp='less than';
      ;;
    '>'|'-gt')
      fc_op='-gt';
      fc_comp='more than';
      ;;
    '!='|'-ne')
      fc_op='-ne';
      fc_comp='not';
      ;;
### func_check()
    *)
      error \
        'func_check(): second argument is not a relational operator.';
      ;;
    esac;
    shift;
    shift;
    shift;
    if test "$#" "${fc_op}" "${fc_nargs}"
    then
      do_nothing;
    else
      error "func_check(): \
${fc_fname}"'() needs '"${fc_comp} ${fc_nargs}"' argument'"${fc_s}"'.';
    fi;
    func_push "${fc_fname}";
    if test _"${_DEBUG_STACKS}"_ = _yes_
    then
      echo2 '+++ '"${fc_fname} $@";
      echo2 '>>> '"${_FUNC_STACK}";
    fi;
    eval ${_UNSET} fc_comp;
    eval ${_UNSET} fc_fname;
    eval ${_UNSET} fc_nargs;
    eval ${_UNSET} fc_op;
    eval ${_UNSET} fc_s;
  } # func_check()


  #############
  # func_pop ()
  #
  # Retrieve the top element from the function stack.  This is called
  # by every return variable in each function.
  #
  # The stack elements are separated by `!'; the popped element is
  # identical to the original element, except that all `!' characters
  # were removed.
  #
  # Arguments: 1
  #
  func_pop()
  {
    if test "$#" -ne 0
    then
      error 'func_pop() does not have arguments.';
    fi;
    case "${_FUNC_STACK}" in
    '')
      if test _"${_DEBUG_STACKS}"_ = _yes_
      then
        error 'func_pop(): stack is empty.';
      fi;
      ;;
    *!*)
      # split at first bang `!'.
      _FUNC_STACK="$(echo1 "${_FUNC_STACK}" | sed 's/^[^!]*!//')";
      exit_test;
      ;;
    *)
      _FUNC_STACK='';
      ;;
    esac;
    if test _"${_DEBUG_STACKS}"_ = _yes_
    then
      echo2 '<<< '"${_FUNC_STACK}";
    fi;
  } # func_pop()


  #############
  # func_push (<element>)
  #
  # Store another element to the function stack.  This is called by
  # func_check() at the beginning of each function.
  #
  # The stack elements are separated by `!'; if <element> contains a `!'
  # it is removed first.
  #
  # Arguments: 1
  #
  # Variable prefix: fp
  #
  func_push()
  {
    if test "$#" -ne 1
    then
      error 'func_push() needs 1 argument.';
    fi;
    case "$1" in
    *'!'*)
      # remove all bangs `!'.
      fp_element="$(echo1 "$1" | sed 's/!//g')";
      exit_test;
      ;;
    *)
      fp_element="$1";
      ;;
    esac;
    if test _"${_FUNC_STACK}"_ = __
    then
      _FUNC_STACK="${fp_element}";
    else
      _FUNC_STACK="${fp_element}!${_FUNC_STACK}";
    fi;
    eval ${_UNSET} fp_element;
  } # func_push()


  #############
  # func_stack_dump ()
  #
  # Print the content of the function stack.  Ignore the arguments.
  #
  func_stack_dump()
  {
    diag 'call stack(): '"${_FUNC_STACK}";
  } # func_stack_dump()

else				# $_DEBUG_FUNC_CHECK is not `yes'

  func_check() { return; }
  func_pop() { return; }
  func_push() { return; }
  func_stack_dump() { return; }

fi;				# test of $_DEBUG_FUNC_CHECK


########################################################################
# get_first_essential (<arg>*)
#
# Retrieve first non-empty argument.
#
# Return  : `1' if all arguments are empty, `0' if found.
# Output  : the retrieved non-empty argument.
#
# Variable prefix: gfe
#
get_first_essential()
{
  func_check get_first_essential '>=' 0 "$@";
  if is_equal "$#" 0
  then
    eval "${return_ok}";
  fi;
  for i
  do
    gfe_var="$i";
    if obj gfe_var is_not_empty
    then
      obj gfe_var echo1;
      eval ${_UNSET} gfe_var;
      eval "${return_ok}";
    fi;
  done;
  eval ${_UNSET} gfe_var;
  eval "${return_bad}";
} # get_first_essential()


########################################################################
landmark '5: is_*()';
########################################################################

########################################################################
# is_dir (<name>)
#
# Test whether `name' is a readable directory.
#
# Arguments : 1
# Return    : `0' if arg1 is a directory, `1' otherwise.
#
is_dir()
{
  func_check is_dir '=' 1 "$@";
  if is_not_empty "$1" && test -d "$1" && test -r "$1"
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_dir()


########################################################################
# is_empty (<string>)
#
# Test whether <string> is empty.
#
# Arguments : <=1
# Return    : `0' if arg1 is empty or does not exist, `1' otherwise.
#
is_empty()
{
  func_check is_empty '=' 1 "$@";
  if test _"$1"_ = __
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_empty()


########################################################################
# is_empty_file (<file_name>)
#
# Test whether <file_name> is an empty existing file.
#
# Arguments : <=1
# Return    :
#   `0' if arg1 is an empty existing file
#   `1' otherwise
#
is_empty_file()
{
  func_check is_empty_file '=' 1 "$@";
  if is_file "$1"
  then
    if test -s "$1"
    then
      eval "${return_no}";
    else
      eval "${return_yes}";
    fi;
  fi;
  eval "${return_no}";
} # is_empty_file()


########################################################################
# is_equal (<string1> <string2>)
#
# Test whether <string1> is equal to <string2>.
#
# Arguments : 2
# Return    : `0' both arguments are equal strings, `1' otherwise.
#
is_equal()
{
  func_check is_equal '=' 2 "$@";
  if test _"$1"_ = _"$2"_
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_equal()


########################################################################
# is_existing (<name>)
#
# Test whether <name> is an existing file or directory.  Solaris 2.5 does
# not have `test -e'.
#
# Arguments : 1
# Return    : `0' if arg1 exists, `1' otherwise.
#
is_existing()
{
  func_check is_existing '=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_no}";
  fi;
  if test -f "$1" || test -d "$1" || test -c "$1"
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_existing()


########################################################################
# is_file (<name>)
#
# Test whether <name> is a readable file.
#
# Arguments : 1
# Return    : `0' if arg1 is a readable file, `1' otherwise.
#
is_file()
{
  func_check is_file '=' 1 "$@";
  if is_not_empty "$1" && test -f "$1" && test -r "$1"
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_file()


########################################################################
# is_greater_than (<integer1> <integer2>)
#
# Test whether <integer1> is greater than <integer2>.
#
# Arguments : 2
# Return    : `0' if <integer1> is a greater integer than <integer2>,
#             `1' otherwise.
#
is_greater_than()
{
  func_check is_greater_than '=' 2 "$@";
  if is_integer "$1" && is_integer "$2" && test "$1" -gt "$2"
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_greater_than()


########################################################################
# is_integer (<string>)
#
# Test whether `string' is an integer.
#
# Arguments : 1
# Return    : `0' if argument is an integer, `1' otherwise.
#
is_integer()
{
  func_check is_integer '=' 1 "$@";
  if is_equal "$(echo1 "$1" | sed -n '
s/^[0-9][0-9]*$/ok/p
s/^[-+][0-9][0-9]*$/ok/p
')" 'ok'
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_integer()


########################################################################
# is_not_empty_file (<file_name>)
#
# Test whether <file_name> is a non-empty existing file.
#
# Arguments : <=1
# Return    :
#   `0' if arg1 is a non-empty existing file
#   `1' otherwise
#
is_not_empty_file()
{
  func_check is_not_empty_file '=' 1 "$@";
  if is_file "$1" && test -s "$1"
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_not_empty_file()


########################################################################
# is_not_dir (<name>)
#
# Test whether <name> is not a readable directory.
#
# Arguments : 1
# Return    : `0' if arg1 is a directory, `1' otherwise.
#
is_not_dir()
{
  func_check is_not_dir '=' 1 "$@";
  if is_dir "$1"
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_dir()


########################################################################
# is_not_empty (<string>)
#
# Test whether <string> is not empty.
#
# Arguments : <=1
# Return    : `0' if arg1 exists and is not empty, `1' otherwise.
#
is_not_empty()
{
  func_check is_not_empty '=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_empty()


########################################################################
# is_not_equal (<string1> <string2>)
#
# Test whether <string1> differs from <string2>.
#
# Arguments : 2
#
is_not_equal()
{
  func_check is_not_equal '=' 2 "$@";
  if is_equal "$1" "$2"
  then
    eval "${return_no}";
  fi
  eval "${return_yes}";
} # is_not_equal()


########################################################################
# is_not_file (<filename>)
#
# Test whether <filename> is a not readable file.
#
# Arguments : 1 (empty allowed)
#
is_not_file()
{
  func_check is_not_file '=' 1 "$@";
  if is_file "$1"
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_file()


########################################################################
# is_not_prog (<program>)
#
# Verify that <program> is not a command in $PATH.
#
# Arguments : 1,  <program> can have spaces and arguments.
#
is_not_prog()
{
  func_check is_not_prog '=' 1 "$@";
  if where_is_prog "$1" >${_NULL_DEV}
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_prog()


########################################################################
# is_not_writable (<name>)
#
# Test whether <name> is not a writable file or directory.
#
# Arguments : >=1 (empty allowed), more args are ignored
#
is_not_writable()
{
  func_check is_not_writable '>=' 1 "$@";
  if is_writable "$1"
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_writable()


########################################################################
# is_not_X ()
#
# Test whether the script is not running in X Window by checking $DISPLAY.
#
is_not_X()
{
  func_check is_not_X '=' 0 "$@";
  if obj DISPLAY is_empty
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_not_X()


########################################################################
# is_not_yes (<string>)
#
# Test whether <string> is not `yes'.
#
# Arguments : 1
#
is_not_yes()
{
  func_check is_not_yes = 1 "$@";
  if is_yes "$1"
  then
    eval "${return_no}";
  fi;
  eval "${return_yes}";
} # is_not_yes()


########################################################################
# is_prog (<name>)
#
# Determine whether <name> is a program in $PATH.
#
# Arguments : 1,  <program> can have spaces and arguments.
#
is_prog()
{
  func_check is_prog '=' 1 "$@";
  if where_is_prog "$1" >${_NULL_DEV}
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_prog()


########################################################################
# is_writable (<name>)
#
# Test whether <name> is a writable file or directory.
#
# Arguments : >=1 (empty allowed), more args are ignored
#
is_writable()
{
  func_check is_writable '>=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_no}";
  fi;
  if test -r "$1"
  then
    if test -w "$1"
    then
      eval "${return_yes}";
    fi;
  fi;
  eval "${return_no}";
} # is_writable()


########################################################################
# is_X ()
#
# Test whether the script is running in X Window by checking $DISPLAY.
#
is_X()
{
  func_check is_X '=' 0 "$@";
  if obj DISPLAY is_not_empty
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_X()


########################################################################
# is_yes (<string>)
#
# Test whether <string> has value `yes'.
#
# Return    : `0' if arg1 is `yes', `1' otherwise.
#
is_yes()
{
  func_check is_yes '=' 1 "$@";
  if is_equal "$1" 'yes'
  then
    eval "${return_yes}";
  fi;
  eval "${return_no}";
} # is_yes()


########################################################################
# landmark ()
#
# Print debugging information on standard error if $_DEBUG_LM is `yes'.
#
# Globals: $_DEBUG_LM
#
# Defined in section `Debugging functions'.


########################################################################
# leave ([<code>])
#
# Clean exit without an error or with error <code>.
#
leave()
{
  clean_up;
  if test $# = 0
  then
    exit "${_OK}";
  else
    exit "$1";
  fi;
} # leave()


########################################################################
landmark '6: list_*()';
########################################################################
#
# `list' is an object class that represents an array or list.  Its
# data consists of space-separated single-quoted elements.  So a list
# has the form "'first' 'second' '...' 'last'".  See list_append() for
# more details on the list structure.  The array elements of `list'
# can be get by `eval set x "$list"; shift`.


########################################################################
# list_append (<list> <element>...)
#
# Add one or more elements to an existing list.  <list> may also be
# empty.
#
# Arguments: >=2
#   <list>: a variable name for a list of single-quoted elements
#   <element>:  some sequence of characters.
# Output: none, but $<list> is set to
#   if <list> is empty:  "'<element>' '...'"
#   otherwise:           "$list '<element>' ..."
#
# Variable prefix: la
#
list_append()
{
  func_check list_append '>=' 2 "$@";
  la_name="$1";
  eval la_list='"${'"$1"'}"';
  shift;
  for s
  do
    la_s="$s";
    case "${la_s}" in
    *\'*)
      # escape each single quote by replacing each
      # "'" (squote) by "'\''" (squote bslash squote squote);
      # note that the backslash must be doubled in the following `sed'
      la_element="$(echo1 "${la_s}" | sed 's/'"${_SQ}"'/&\\&&/g')";
      exit_test;
      ;;
    '')
      la_element="";
      ;;
    *)
      la_element="${la_s}";
      ;;
    esac;
### list_append()
    if obj la_list is_empty
    then
      la_list="'${la_element}'";
    else
      la_list="${la_list} '${la_element}'";
    fi;
  done;
  eval "${la_name}"='"${la_list}"';
  eval ${_UNSET} la_element;
  eval ${_UNSET} la_list;
  eval ${_UNSET} la_name;
  eval ${_UNSET} la_s;
  eval "${return_ok}";
} # list_append()


########################################################################
# list_from_cmdline (<pre_name_of_opt_lists> [<cmdline_arg>...])
#
# Transform command line arguments into a normalized form.
#
# Options, option arguments, and file parameters are identified and
# output each as a single-quoted argument of its own.  Options and
# file parameters are separated by a '--' argument.
#
# Arguments: >=1
#   <pre_name>: common part of a set of 4 environment variable names:
#     $<pre_name>_SHORT_NA:  list of short options without an arg.
#     $<pre_name>_SHORT_ARG: list of short options that have an arg.
#     $<pre_name>_LONG_NA:   list of long options without an arg.
#     $<pre_name>_LONG_ARG:  list of long options that have an arg.
#   <cmdline_arg>...: the arguments from a command line, such as "$@",
#                     the content of a variable, or direct arguments.
#
# Output: ['-[-]opt' ['optarg']]... '--' ['filename']...
#
# Example:
#   list_from_cmdline PRE -a f1 -bcarg --lon=larg f2 low larg2
#     PRE_SHORT_NA="'a' 'b'"
#     PRE_SHORT_ARG="'c' 'd'"
#     PRE_LONG_NA="'help' 'version'"
#     PRE_LONG_ARG="'longer' 'lower'"
# This will result in printing:
#   '-a' '-b' '-c' 'arg' '--longer' 'larg' '--lower' 'larg2' '--' 'f1' 'f2'
#
#   Use this function in the following way:
#     eval set x "$(list_from_cmdline PRE_NAME "$@")";
#     shift;
#     while test "$1" != '--'; do
#       case "$1" in
#       ...
#       esac;
#       shift;
#     done;
#     shift;         #skip '--'
#     # all positional parameters ("$@") left are file name parameters.
#
# Variable prefix: lfc
#
list_from_cmdline()
{
  func_check list_from_cmdline '>=' 1 "$@";
  # short options, no argument
  obj_from_output lfc_short_n obj_data "$1"_SHORT_NA;
  # short options, with argument
  obj_from_output lfc_short_a obj_data "$1"_SHORT_ARG;
  # long options, no argument
  obj_from_output lfc_long_n obj_data "$1"_LONG_NA;
  # long options, with argument
  obj_from_output lfc_long_a obj_data "$1"_LONG_ARG;
  if obj lfc_short_n is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_SHORT_NA options.';
  fi;
  if obj lfc_short_a is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_SHORT_ARG options.';
  fi;
  if obj lfc_long_n is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_LONG_NA options.';
  fi;
  if obj lfc_long_a is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_LONG_ARG options.';
  fi;
  shift;

  if is_equal "$#" 0
  then
    echo1 "'--'"
    eval ${_UNSET} lfc_fparams;
    eval ${_UNSET} lfc_short_a;
    eval ${_UNSET} lfc_short_n;
### list_from_cmdline()
    eval ${_UNSET} lfc_long_a;
    eval ${_UNSET} lfc_long_n;
    eval ${_UNSET} lfc_result;
    eval "${return_ok}";
  fi;

  lfc_fparams='';
  lfc_result='';
  while is_greater_than "$#" 0
  do
    lfc_arg="$1";
    shift;
    case "${lfc_arg}" in
    --) break; ;;
    --*=*)
      # delete leading '--';
      lfc_with_equal="$(echo1 "${lfc_arg}" | sed 's/^--//')";
      # extract option by deleting from the first '=' to the end
      lfc_abbrev="$(echo1 "${lfc_with_equal}" | \
                    sed 's/^\([^=]*\)=.*$/\1/')";
      obj_from_output lfc_opt \
        list_single_from_abbrev lfc_long_a "${lfc_abbrev}";
      if obj lfc_opt is_empty
      then
        error_user "--${lfc_abbrev} is not an option.";
      else
        # get the option argument by deleting up to first `='
        lfc_optarg="$(echo1 "${lfc_with_equal}" | sed 's/^[^=]*=//')";
        exit_test;
        list_append lfc_result "--${lfc_opt}" "${lfc_optarg}";
        continue;
      fi;
### list_from_cmdline()
      ;;
    --*)
      # delete leading '--';
      lfc_abbrev="$(echo1 "${lfc_arg}" | sed 's/^--//')";
      if list_has lfc_long_n "${lfc_abbrev}"
      then
        lfc_opt="${lfc_abbrev}";
      else
        obj_from_output lfc_opt \
          list_single_from_abbrev lfc_long_n "${lfc_abbrev}";
        if obj lfc_opt is_not_empty && is_not_equal "$#" 0
        then
          obj_from_output a \
            list_single_from_abbrev lfc_long_a "${lfc_abbrev}";
          if obj a is_not_empty
          then
            error_user "The abbreviation ${lfc_arg} \
has multiple options: --${lfc_opt} and --${a}.";
          fi;
        fi;
      fi; # if list_has lfc_long_n "${lfc_abbrev}"
      if obj lfc_opt is_not_empty
      then
        # long option, no argument
        list_append lfc_result "--${lfc_opt}";
        continue;
      fi;
      obj_from_output lfc_opt \
        list_single_from_abbrev lfc_long_a "${lfc_abbrev}";
      if obj lfc_opt is_not_empty
      then
### list_from_cmdline()
        # long option with argument
        if is_equal "$#" 0
        then
          error_user "no argument for option --${lfc_opt}."
        fi;
        list_append lfc_result "--${lfc_opt}" "$1";
        shift;
        continue;
      fi; # if obj lfc_opt is_not_empty
      error_user "${lfc_arg} is not an option.";
      ;;
    -?*)			# short option (cluster)
      # delete leading `-';
      lfc_rest="$(echo1 "${lfc_arg}" | sed 's/^-//')";
      exit_test;
      while obj lfc_rest is_not_empty
      do
        # get next short option from cluster (first char of $lfc_rest)
        lfc_optchar="$(echo1 "${lfc_rest}" | sed 's/^\(.\).*$/\1/')";
        # remove first character from ${lfc_rest};
        lfc_rest="$(echo1 "${lfc_rest}" | sed 's/^.//')";
        exit_test;
        if list_has lfc_short_n "${lfc_optchar}"
        then
          list_append lfc_result "-${lfc_optchar}";
          continue;
        elif list_has lfc_short_a "${lfc_optchar}"
        then
          if obj lfc_rest is_empty
          then
            if is_greater_than "$#" 0
            then
### list_from_cmdline()
              list_append lfc_result "-${lfc_optchar}" "$1";
              shift;
              continue;
            else
              error_user "no argument for option -${lfc_optchar}.";
            fi;
          else			# rest is the argument
            list_append lfc_result "-${lfc_optchar}" "${lfc_rest}";
            lfc_rest='';
            continue;
          fi; # if obj lfc_rest is_empty
        else
          error_user "unknown option -${lfc_optchar}.";
        fi; # if list_has lfc_short_n "${lfc_optchar}"
      done; # while obj lfc_rest is_not_empty
      ;;
    *)
      # Here, $lfc_arg is not an option, so a file parameter.
      list_append lfc_fparams "${lfc_arg}";

      # Ignore the strange POSIX option handling to end option
      # parsing after the first file name argument.  To reuse it, do
      # a `break' here if $POSIXLY_CORRECT of `bash' is not empty.
      # When `bash' is called as `sh' $POSIXLY_CORRECT is set
      # automatically to `y'.
      ;;
    esac; # case "${lfc_arg}" in
  done; # while is_greater_than "$#" 0
  list_append lfc_result '--';
  if obj lfc_fparams is_not_empty
  then
    lfc_result="${lfc_result} ${lfc_fparams}";
  fi;
### list_from_cmdline()
  if is_greater_than "$#" 0
  then
    list_append lfc_result "$@";
  fi;
  obj lfc_result echo1;
  eval ${_UNSET} lfc_abbrev;
  eval ${_UNSET} lfc_fparams;
  eval ${_UNSET} lfc_short_a;
  eval ${_UNSET} lfc_short_n;
  eval ${_UNSET} lfc_long_a;
  eval ${_UNSET} lfc_long_n;
  eval ${_UNSET} lfc_result;
  eval ${_UNSET} lfc_arg;
  eval ${_UNSET} lfc_opt;
  eval ${_UNSET} lfc_opt_arg;
  eval ${_UNSET} lfc_opt_char;
  eval ${_UNSET} lfc_with_equal;
  eval ${_UNSET} lfc_rest;
  eval "${return_ok}";
} # list_from_cmdline()


########################################################################
# list_from_cmdline_with_minus (<pre_name_of_opt_lists> [<cmdline_arg>...])
#
# Transform command line arguments into a normalized form with a double
# abbreviation before and after an internal `-' sign.
#
# Options, option arguments, and file parameters are identified and
# output each as a single-quoted argument of its own.  Options and
# file parameters are separated by a `--' argument.
#
# Arguments: >=1
#   <pre_name>: common part of a set of 4 environment variable names:
#     $<pre_name>_SHORT_NA:  list of short options without an arg.
#     $<pre_name>_SHORT_ARG: list of short options that have an arg.
#     $<pre_name>_LONG_NA:   list of long options without an arg.
#     $<pre_name>_LONG_ARG:  list of long options that have an arg.
#   <cmdline_arg>...: the arguments from a command line, such as "$@",
#                     the content of a variable, or direct arguments.
#
# Output: ['-[-]opt' ['optarg']]... '--' ['filename']...
#
# Example:
#   list_from_cmdline PRE -a f1 -bcarg --lon=larg --h-n f2 low larg2
#     PRE_SHORT_NA="'a' 'b'"
#     PRE_SHORT_ARG="'c' 'd'"
#     PRE_LONG_NA="'help' 'version' 'hi-non-arg'"
#     PRE_LONG_ARG="'long-arg' 'low-arg'"
# This will result in printing:
#   '-a' '-b' '-c' 'arg' '--long-arg' 'larg' '--hi-non-arg' \
#     '--low-arg' 'larg2' '--' 'f1' 'f2'
#
# Use this function in the following way:
#   eval set x "$(list_from_cmdline_with_minus PRE_NAME "$@")";
#   shift;
#   while test "$1" != '--'; do
#     case "$1" in
#     ...
#     esac;
#     shift;
#   done;
#   shift;         #skip '--'
#   # all positional parameters ("$@") left are file name parameters.
#
# Variable prefix: lfcwm
#
list_from_cmdline_with_minus()
{
  func_check list_from_cmdline_with_minus '>=' 1 "$@";
  # short options, no argument
  obj_from_output lfcwm_short_n obj_data "$1"_SHORT_NA;
  # short options, with argument
  obj_from_output lfcwm_short_a obj_data "$1"_SHORT_ARG;
  # long options, no argument
  obj_from_output lfcwm_long_n obj_data "$1"_LONG_NA;
  # long options, with argument
  obj_from_output lfcwm_long_a obj_data "$1"_LONG_ARG;
  if obj lfcwm_short_n is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_SHORT_NA options.';
  fi;
  if obj lfcwm_short_a is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_SHORT_ARG options.';
  fi;
  if obj lfcwm_long_n is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_LONG_NA options.';
  fi;
  if obj lfcwm_long_a is_empty
  then
    error 'list_from_cmdline(): no $'"$1"'_LONG_ARG options.';
  fi;
  shift;

  if is_equal "$#" 0
  then
    echo1 "'--'";
    eval ${_UNSET} lfcwm_short_a;
    eval ${_UNSET} lfcwm_short_n;
### list_from_cmdline_with_minus()
    eval ${_UNSET} lfcwm_long_a;
    eval ${_UNSET} lfcwm_long_n;
    eval "${return_ok}";
  fi;
  obj_from_output lfcwm_long_both lists_combine lfcwm_long_a lfcwm_long_n;
  lfcwm_fparams='';
  lfcwm_result='';
  while is_greater_than "$#" 0	# command line arguments
  do
    lfcwm_arg="$1";
    shift;
    lfcwm_optarg='';
    case "${lfcwm_arg}" in
    --)
      break;
      ;;
    --*=*)
      # delete leading '--';
      lfcwm_with_equal="$(echo1 "${lfcwm_arg}" | sed 's/^--//')";
      # extract option by deleting from the first '=' to the end
      lfcwm_abbrev="$(echo1 "${lfcwm_with_equal}" | \
                    sed 's/^\([^=]*\)=.*$/\1/')";
      # extract option argument by deleting up to the first '='
      lfcwm_optarg="$(echo1 "${lfcwm_with_equal}" | \
                    sed 's/^[^=]*=\(.*\)$/\1/')";
### list_from_cmdline_with_minus()
      if list_has lfcwm_long_a "${lfcwm_abbrev}"
      then
        lfcwm_opt="${lfcwm_abbrev}";
      else
        obj_from_output lfcwm_opt \
          _search_abbrev lfcwm_long_a "${lfcwm_abbrev}";
      fi;
      list_append lfcwm_result "--${lfcwm_opt}" "${lfcwm_optarg}";
      continue;
      ;;
    --*)
      # delete leading '--';
      lfcwm_abbrev="$(echo1 "${lfcwm_arg}" | sed 's/^--//')";
      if list_has lfcwm_long_both "${lfcwm_abbrev}"
      then
        lfcwm_opt="${lfcwm_abbrev}";
      else
        obj_from_output lfcwm_opt \
          _search_abbrev lfcwm_long_both "${lfcwm_abbrev}";
      fi;
### list_from_cmdline_with_minus()
      if list_has lfcwm_long_a "${lfcwm_opt}"
      then
        if is_equal "$#" 0
        then
          error_user "Option ${lfcwm_opt} needs an argument.";
        fi;
        lfcwm_optarg="$1";
        shift;
        list_append lfcwm_result "--${lfcwm_opt}" "${lfcwm_optarg}";
      else
        list_append lfcwm_result "--${lfcwm_opt}";
      fi;
      continue;
      ;;
    -?*)			# short option (cluster)
      # delete leading '-';
      lfcwm_rest="$(echo1 "${lfcwm_arg}" | sed 's/^-//')";
      while obj lfcwm_rest is_not_empty
      do
        # get next short option from cluster (first char of $lfcwm_rest)
        lfcwm_optchar="$(echo1 "${lfcwm_rest}" | sed 's/^\(.\).*$/\1/')";
        # remove first character from ${lfcwm_rest};
        lfcwm_rest="$(echo1 "${lfcwm_rest}" | sed 's/^.//')";
        if list_has lfcwm_short_n "${lfcwm_optchar}"
        then
          list_append lfcwm_result "-${lfcwm_optchar}";
          continue;
        elif list_has lfcwm_short_a "${lfcwm_optchar}"
        then
          if obj lfcwm_rest is_empty
          then
            if is_greater_than "$#" 0
            then
### list_from_cmdline_with_minus()
              list_append lfcwm_result "-${lfcwm_optchar}" "$1";
              shift;
              continue;
            else
              error_user "no argument for option -${lfcwm_optchar}.";
            fi;
          else			# rest is the argument
            list_append lfcwm_result "-${lfcwm_optchar}" "${lfcwm_rest}";
            lfcwm_rest='';
            continue;
          fi; # if obj lfcwm_rest is_empty
        else
          error_user "unknown option -${lfcwm_optchar}.";
        fi; # if list_has lfcwm_short_n "${lfcwm_optchar}"
      done; # while obj lfcwm_rest is_not_empty
      ;;
    *)
      # Here, $lfcwm_arg is not an option, so a file parameter.
      list_append lfcwm_fparams "${lfcwm_arg}";

      # Ignore the strange POSIX option handling to end option
      # parsing after the first file name argument.  To reuse it, do
      # a `break' here if $POSIXLY_CORRECT of `bash' is not empty.
      # When `bash' is called as `sh' $POSIXLY_CORRECT is set
      # automatically to `y'.
      ;;
    esac;
  done;

  list_append lfcwm_result '--';
  if obj lfcwm_fparams is_not_empty
  then
    lfcwm_result="${lfcwm_result} ${lfcwm_fparams}";
  fi;
### list_from_cmdline_with_minus()
  if is_greater_than "$#" 0
  then
    list_append lfcwm_result "$@";
  fi;
  obj lfcwm_result echo1;
  eval ${_UNSET} lfcwm_abbrev;
  eval ${_UNSET} lfcwm_fparams;
  eval ${_UNSET} lfcwm_short_a;
  eval ${_UNSET} lfcwm_short_n;
  eval ${_UNSET} lfcwm_long_a;
  eval ${_UNSET} lfcwm_long_both;
  eval ${_UNSET} lfcwm_long_n;
  eval ${_UNSET} lfcwm_result;
  eval ${_UNSET} lfcwm_arg;
  eval ${_UNSET} lfcwm_opt;
  eval ${_UNSET} lfcwm_optarg;
  eval ${_UNSET} lfcwm_optchar;
  eval ${_UNSET} lfcwm_with_equal;
  eval ${_UNSET} lfcwm_rest;
  eval "${return_ok}";
} # list_from_cmdline_with_minus()


# _search_abbrev (<list> <abbrev>)
#
# Check whether <list> has an element constructed from the abbreviation
# <abbrev>.  All `-' in <abbrev> are replaced by `-*'.  This construction
# is searched first with `<construction>[^-]*'.  If there is more than a
# single element an error is created.  If none is found `<construction>*'
# is searched.  Again an error is created for several results.
# This function was constructed from the former function
# list_single_from_abbrev().
#
# This is a local function of list_from_cmdline_with_minus().
#
# Arguments: 2
#   <list>:   a variable name for a list of single-quoted elements
#   <abbrev>: some sequence of characters.
#
# Output: the found element (always not empty), error when none found.
#
# Variable prefix: _sa
#
_search_abbrev()
{
  func_check _search_abbrev '=' 2 "$@";
  eval _sa_list='"${'$1'}"';
  if obj _sa_list is_empty
  then
    error "_search_abbrev(): list is empty.";
  fi;

  _sa_abbrev="$2";
  if obj _sa_abbrev is_empty
  then
    error "_search_abbrev(): abbreviation argument is empty.";
  fi;

  _sa_case="$(echo1 "${_sa_abbrev}" | sed 's/-/\*-/g')";
  _sa_opt='';
  case " ${_sa_list}" in
  *\ \'${_sa_case}*)		# list has the abbreviation
    _sa_m1='';
    _sa_m2='';
    _sa_nm='';
    eval set x "${_sa_list}";
    shift;
    for i			# over the option list
    do
      _sa_i="$i";
### _search_abbrev() of list_from_cmdline_with_minus()
      case "${_sa_i}" in
      ${_sa_case}*-*)
        if obj _sa_m1 is_empty
        then
          _sa_m1="${_sa_i}";
          continue;
        fi;
        _sa_m2="${_sa_i}";
        continue;
        ;;
      ${_sa_case}*)
        if obj _sa_nm is_empty
        then
          _sa_nm="${_sa_i}";
          continue;
        fi;
        error_user "The abbreviation --${_sa_abbrev} has multiple options "\
"--${_sa_nm} and --${_sa_i}.";
        ;;
      esac;
    done;
    if obj _sa_nm is_empty
    then
      if obj _sa_m2 is_not_empty
      then
        error_user "The abbreviation --${_sa_abbrev} has multiple options "\
"--${_sa_m1} and --${_sa_m2}.";
      fi;
### _search_abbrev() of list_from_cmdline_with_minus()
      if obj _sa_m1 is_not_empty
      then
        _sa_opt="${_sa_m1}";
      fi;
    else
      _sa_opt="${_sa_nm}";
    fi;
    ;;
  esac;
  if obj _sa_opt is_empty
  then
    error_user "--${_sa_abbrev} is not an option.";
  fi;
  obj _sa_opt echo1;
  eval "${_UNSET}" _sa_abbrev;
  eval "${_UNSET}" _sa_case;
  eval "${_UNSET}" _sa_i;
  eval "${_UNSET}" _sa_list;
  eval "${_UNSET}" _sa_m1;
  eval "${_UNSET}" _sa_m2;
  eval "${_UNSET}" _sa_nm;
  eval "${_UNSET}" _sa_opt;
  eval "${return_ok}";
} # _search_abbrev() of list_from_cmdline_with_minus()


########################################################################
# list_from_file (<list_name> <file_name>)
#
# Extrect the lines from <file_name> and store them as elements to list
# <list_name>.
#
# Arguments: 2
#   <list_name>: a variable name for output, a list of single-quoted elts
#   <file_name>: the name of an existing file
#
# Variable prefix: lff
#
list_from_file()
{
  func_check list_from_file '=' 2 "$@";
  if is_not_file "$2"
  then
    eval "${return_bad}";
  fi;
  lff_n="$(wc -l "$2" | eval sed "'s/^[ ${_TAB}]*\([0-9]\+\).*$/\1/'")";
  eval "$1"="''";
  if obj lff_n is_equal 0
  then
    eval "${return_good}";
  fi;
  lff_i=0;
  while obj lff_i is_not_equal "${lff_n}"
  do
    lff_i="$(expr "${lff_i}" + 1)";
    list_append "$1" "$(eval sed -n "'${lff_i}p
${lff_i}q'" "'$2'")";
  done;
  eval "${_UNSET}" lff_i;
  eval "${_UNSET}" lff_n;
  eval "${return_good}";
} # list_from_file()


########################################################################
# list_from_split (<string> <separator_char>)
#
# Split <string> by <separator_char> into a list, omitting the separator.
#
# Arguments: 2: a <string> that is to be split into parts divided by
#               character <separator_char>
# Output: the resulting list string
#
# Variable prefix: lfs
#
list_from_split()
{
  func_check list_from_split = 2 "$@";
  if is_empty "$1"
  then
    eval "${return_ok}";
  fi;
  case "$2" in
  ?)
    lfs_splitter="$2";
    ;;
  '\:')
    lfs_splitter=':';
    ;;
  *)
    error "list_from_split(): split argument $2 must be a single character.";
    ;;
  esac;
  lfs_list='';
  lfs_rest="$1";
  while :
  do
    case "${lfs_rest}" in
    *${lfs_splitter}*)
      case "${lfs_splitter}" in
      /)
        lfs_elt="$(echo1 ${lfs_rest} | sed \
          's|^\([^'"${lfs_splitter}"']*\)'"${lfs_splitter}"'.*|\1|')";
        lfs_rest="$(echo1 ${lfs_rest} | sed \
          's|^[^'"${lfs_splitter}"']*'"${lfs_splitter}"'\(.*\)$|\1|')";
        ;;
      *)
### list_from_split()
        lfs_elt="$(echo1 ${lfs_rest} | sed \
          's/^\([^'"${lfs_splitter}"']*\)'"${lfs_splitter}"'.*/\1/')";
        lfs_rest="$(echo1 ${lfs_rest} | sed \
          's/^[^'"${lfs_splitter}"']*'"${lfs_splitter}"'\(.*\)$/\1/')";
        ;;
      esac;
      list_append lfs_list "${lfs_elt}"
      continue;
      ;;
    *)
      list_append lfs_list "${lfs_rest}"
      break
      ;;
    esac;
  done
  echo1 "${lfs_list}";

  eval ${_UNSET} lfs_elt;
  eval ${_UNSET} lfs_list;
  eval ${_UNSET} lfs_rest;
  eval ${_UNSET} lfs_splitter;
  eval "${return_ok}";
} # list_from_split()


########################################################################
# list_has (<list-name> <element>)
#
# Test whether the list <list-name> has the element <element>.
#
# Arguments: 2
#   <list_name>: a variable name for a list of single-quoted elements
#   <element>:  some sequence of characters.
#
# Variable prefix: lh
#
list_has()
{
  func_check list_has = 2 "$@";
  eval lh_list='"${'$1'}"';
  if obj lh_list is_empty
  then
    eval "${_UNSET}" lh_list;
    eval "${return_no}";
  fi;
  case "$2" in
    \'*\')  lh_element=" $2 "; ;;
    *)      lh_element=" '$2' "; ;;
  esac;
  if string_contains " ${lh_list} " "${lh_element}"
  then
    eval "${_UNSET}" lh_list;
    eval "${_UNSET}" lh_element;
    eval "${return_yes}";
  else
    eval "${_UNSET}" lh_list;
    eval "${_UNSET}" lh_element;
    eval "${return_no}";
  fi;
} # list_has()


########################################################################
# list_has_abbrev (<list_var> <abbrev>)
#
# Test whether the list of <list_var> has an element starting with
# <abbrev>.
#
# Arguments: 2
#   <list_var>: a variable name for a list of single-quoted elements
#   <abbrev>:   some sequence of characters.
#
# Variable prefix: lha
#
list_has_abbrev()
{
  func_check list_has_abbrev '=' 2 "$@";
  eval lha_list='"${'$1'}"';
  if obj lha_list is_empty
  then
    eval "${_UNSET}" lha_list;
    eval "${return_no}";
  fi;
  case "$2" in
  \'*)
    lha_element="$(echo1 "$2" | sed 's/'"${_SQ}"'$//')";
    ;;
  *)
    lha_element="'$2";
    ;;
  esac;
  if string_contains " ${lha_list}" " ${lha_element}"
  then
    eval "${_UNSET}" lha_list;
    eval "${_UNSET}" lha_element;
    eval "${return_yes}";
  else
    eval "${_UNSET}" lha_list;
    eval "${_UNSET}" lha_element;
    eval "${return_no}";
  fi;
  eval "${return_ok}";
} # list_has_abbrev()


########################################################################
# list_has_not (<list> <element>)
#
# Test whether <list> has no <element>.
#
# Arguments: 2
#   <list>:    a space-separated list of single-quoted elements.
#   <element>: some sequence of characters.
#
# Variable prefix: lhn
#
list_has_not()
{
  func_check list_has_not = 2 "$@";
  eval lhn_list='"${'$1'}"';
  if obj lhn_list is_empty
  then
    eval "${_UNSET}" lhn_list;
    eval "${return_yes}";
  fi;
  case "$2" in
    \'*\') lhn_element=" $2 "; ;;
    *)     lhn_element=" '$2' "; ;;
  esac;
  if string_contains " ${lhn_list} " "${lhn_element}"
  then
    eval "${_UNSET}" lhn_list;
    eval "${_UNSET}" lhn_element;
    eval "${return_no}";
  else
    eval "${_UNSET}" lhn_list;
    eval "${_UNSET}" lhn_element;
    eval "${return_yes}";
  fi;
} # list_has_not()


########################################################################
# list_single_from_abbrev (<list-var> <abbrev>)
#
# Check whether the list has an element starting with <abbrev>.  If
# there are more than a single element an error is raised.
#
# Arguments: 2
#   <list-var>: a variable name for a list of single-quoted elements
#   <abbrev>:   some sequence of characters.
#
# Output: the found element.
#
# Variable prefix: lsfa
#
list_single_from_abbrev()
{
  func_check list_single_from_abbrev '=' 2 "$@";
  eval lsfa_list='"${'$1'}"';
  if obj lsfa_list is_empty
  then
    eval "${_UNSET}" lsfa_list;
    eval "${return_no}";
  fi;
  lsfa_abbrev="$2";
  if list_has lsfa_list "${lsfa_abbrev}"
  then
    obj lsfa_abbrev echo1;
    eval "${_UNSET}" lsfa_abbrev;
    eval "${_UNSET}" lsfa_list;
    eval "${return_yes}";
  fi;
  if list_has_abbrev lsfa_list "${lsfa_abbrev}"
  then
    lsfa_element='';
    eval set x "${lsfa_list}";
    shift;
### list_single_from_abbrev()
    for i
    do
      case "$i" in
      ${lsfa_abbrev}*)
        if obj lsfa_element is_not_empty
        then
          error_user "The abbreviation --${lsfa_abbrev} \
has multiple options: --${lsfa_element} and --${i}.";
        fi;
        lsfa_element="$i";
        ;;
      esac;
    done;
    obj lsfa_element echo1;
    eval "${_UNSET}" lsfa_abbrev;
    eval "${_UNSET}" lsfa_element;
    eval "${_UNSET}" lsfa_list;
    eval "${return_yes}";
  else
    eval "${_UNSET}" lsfa_abbrev;
    eval "${_UNSET}" lsfa_element;
    eval "${_UNSET}" lsfa_list;
    eval "${return_no}";
  fi;
} # list_single_from_abbrev()


########################################################################
# list_uniq (<list>)
#
# Generate a list with only unique elements.
#
# Output: the corrected list
#
# Variable prefix: lu
#
list_uniq()
{
  func_check list_uniq '=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_ok}";
  fi;
  eval a='"${'"$1"'}"';
  if obj a is_empty
  then
    eval "${return_ok}";
  fi;
  eval set x "$a";
  shift;
  lu_list='';
  for i
  do
    lu_i="$i";
    if list_has lu_list "${lu_i}"
    then
      continue;
    else
      list_append lu_list ${lu_i};
    fi;
  done;
  obj lu_list echo1;
  eval "${_UNSET}" lu_i;
  eval "${_UNSET}" lu_list;
  eval "${return_ok}";
} # list_uniq()


########################################################################
# lists_combine (<list1> <list2> ...)
#
# Combine several lists to a single list.  All arguments are list names.
#
# Output: the combined list
#
# Variable prefix: lc
#
lists_combine()
{
  func_check lists_combine '>=' 2 "$@";
  lc_list='';
  for i
  do
    eval lc_arg='"${'"$i"'}"';
    case "${lc_arg}" in
    '') :; ;;
    "'"*"'")
      if obj lc_list is_empty
      then
        lc_list="${lc_arg}";
      else
        lc_list="${lc_list} ${lc_arg}";
      fi;
      ;;
    *)
      error 'lists_combine(): $'"$i"' is not a list.';
      ;;
    esac;
  done;
  obj lc_list echo1;
  eval "${_UNSET}" lc_arg;
  eval "${_UNSET}" lc_list;
  eval "${return_ok}";
} # lists_combine()


########################################################################
landmark '7: man_*()';
########################################################################

########################################################################
# Information on the search of man pages in groffer

# The search of man pages is based on a set of directories.  That
# starts with the so-called man path.  This is determined in function
# man_setup() either by the command-line option --manpath, by $MANOPT,
# or by $MANPATH.  There is also a program `manpath'.  If all of this
# does not work a man path is created from $PATH with function
# manpath_set_from_path().  We now have a set of existing directories
# for the search of man pages; usually they end with `/man'.

# The directory set of the man path can be changed in 2 ways.  If
# operating system names are given in $SYSTEM or by --systems on the
# command-line all man path directory will be appended by these names.
# The appended system names replace the original man path; but if no
# system name is set, the original man path is kept.  In `groffer',
# this is done by the function manpath_add_lang_sys() in man_setup().

# The next addition for directories is the language.  It is specified
# by --locale or by one of the environment variables $LC_ALL,
# $LC_MESSAGES, and $LANG.  The language name of `C' or `POSIX' means
# the return to the default language (usually English); this deletes
# former language specifications.  The language name and its
# abbreviation with 2 characters is appended to the man page
# directories.  But these new arising directories are added to the man
# page, they do not replace it such as the system names did.  This is
# done by function manpath_add_lang_sys() in man_setup() as well.

# Now we have the basic set of directories for the search of man pages
# for given filespec arguments.  The real directories with the man
# page source files are gotten by appending `man<section>' to each
# directory, where section is a single character of the form
# `[1-9on]'.

# There you find files named according to the form
# <name>.<section>[<extension>][<compression>], where `[]' means
# optional this time.  <name> is the name of the man page; <section>
# is the single character from the last paragraphe; the optional
# <extension> consists of some letters denoting special aspects for
# the section; and the optional <compression> is something like `.gz',
# `.Z', or `.bz2', meaning that the file is compressed.

# If name, section. and extension are specified on the command-line
# the file of the form <name>.<section><extension> with or without
# <compression> are handled.  The first one found according to the
# directory set for the section is shown.

# If just name and section are specified on the command-line then
# first <name>.<section> with or without <compression> are searched.
# If no matching file was found, <name>.<section><extension> with or
# without <compression> are searched for all possible extensions.

# If only name is specified on the command-line then the section
# directories are searched by and by, starting with section `1', until
# a file is matched.

# The function man_is_man() determines all suitable man files for a
# command-line argument, while man_get() searches the single matching
# file for display.


########################################################################
# man_get (<man-name> [<section> [<extension>]])
#
# Write a man page to the temporary file.
#
# Globals in: $_TMP_MANSPEC, $_MAN_SEC_CHARS, $_MAN_EXT, $_MAN_ALL
#
# Arguments: 1, 2, or 3
#
# Variable prefix: mg
#
man_get()
{
  func_check man_get '>=' 1 "$@";
  if obj _TMP_MANSPEC is_empty
  then
    error 'man_get(): man_is_man() must be run first on '"$*".;
  fi;
  mg_name="$1";
  mg_sec="$2";
  if is_empty "$2"
  then
    mg_sec="${_MAN_SEC_CHARS}";	# take care it is not a single section
  fi;
  mg_ext="$3";
  if is_empty "$3"
  then
    mg_ext="${_MAN_EXT}";
  fi;
  if obj _TMP_MANSPEC is_not_equal "${_TMP_DIR}/,man:$1:${mg_sec}${mg_ext}"
  then
    error 'man_get(): $_TMP_MANSPEC does not suit to the arguments '"$*".;
  fi;
### man_get()

  if obj _MAN_ALL is_yes
  then
    list_from_file mg_list "${_TMP_MANSPEC}";
    eval set x ${mg_list};
    shift;
    mg_ok='no';
    mg_list='';
    for f
    do
      mg_f="$f";
      if list_has mg_list "${mg_f}"
      then
        continue;
      else
        list_append mg_list "${mg_f}";
      fi;
### man_get()
      if obj mg_f is_file
      then
        to_tmp "${mg_f}" && mg_ok='yes';
      fi;
    done;
    if obj mg_ok is_yes
    then
      register_title man:"${mg_name}";
    fi;
    eval ${_UNSET} mg_ext;
    eval ${_UNSET} mg_f;
    eval ${_UNSET} mg_list;
    eval ${_UNSET} mg_name;
    eval ${_UNSET} mg_sec;
    eval "${return_ok}";
  else				# $_MAN_ALL is not 'yes'
    if is_empty "$2"
    then			# no section from command line
      if obj _MAN_SEC_LIST is_empty
      then
        m="${_MAN_AUTO_SEC_LIST}"; # list of all sections
      else
        m="${_MAN_SEC_LIST}";	# from --sections
      fi;
### man_get()
      for s in $(eval set x $m; shift; echo1 "$@")
      do
        mg_s="$s";
        list_from_file mg_list "${_TMP_MANSPEC}";
        eval set x ${mg_list};
        shift;
        if obj mg_ext is_empty
        then
          for f
          do
            mg_f="$f";
            case "${mg_f}" in
*/man"${mg_s}"/"${mg_name}"."${mg_s}"|*/man"${mg_s}"/"${mg_name}"."${mg_s}".*)
              if obj mg_f is_file
              then
                to_tmp "${mg_f}" && register_title "${mg_name}(${mg_s})";
                eval ${_UNSET} mg_ext;
                eval ${_UNSET} mg_f;
                eval ${_UNSET} mg_list;
                eval ${_UNSET} mg_name;
                eval ${_UNSET} mg_s;
                eval ${_UNSET} mg_sec;
                eval "${return_ok}";
              fi;
              ;;
             esac;		# "$mg_f"
          done;			# for f
        fi;			# mg_ext is_empty
### man_get()
        for f
        do
          mg_f="$f";
          case "${mg_f}" in
          */man"${mg_s}"/"${mg_name}"."${mg_s}""${mg_ext}"*)
            if obj mg_f is_file
            then
              to_tmp "${mg_f}" && register_title "${mg_name}(${mg_s})";
              eval ${_UNSET} mg_ext;
              eval ${_UNSET} mg_f;
              eval ${_UNSET} mg_list;
              eval ${_UNSET} mg_name;
              eval ${_UNSET} mg_s;
              eval ${_UNSET} mg_sec;
              eval "${return_ok}";
            fi;
            ;;
           esac;		# "$mg_f"
        done;			# for f
      done;			# for s
    else			# $mg_sec is not empty, do with section
      list_from_file mg_list "${_TMP_MANSPEC}";
      eval set x ${mg_list};
      shift;
      if obj mg_ext is_empty
      then
        for f
        do
          mg_f="$f";
### man_get()
          case "${mg_f}" in
*/man"${mg_sec}"/"${mg_name}"."${mg_sec}"|\
*/man"${mg_sec}"/"${mg_name}"."${mg_sec}".*)
            if obj mg_f is_file
            then
              obj mg_f to_tmp && \
                register_title "${mg_name}(${mg_sec})";
              eval ${_UNSET} mg_ext;
              eval ${_UNSET} mg_f;
              eval ${_UNSET} mg_list;
              eval ${_UNSET} mg_name;
              eval ${_UNSET} mg_s;
              eval ${_UNSET} mg_sec;
              eval "${return_ok}";
            fi;
            ;;
          esac;
        done;			# for f
        for f
        do
          mg_f="$f";
### man_get()
          case "${mg_f}" in
*/man"${mg_sec}"/"${mg_name}"."${mg_sec}"*)
            if obj mg_f is_file
            then
              obj mg_f to_tmp && \
                register_title "${mg_name}(${mg_sec})";
              eval ${_UNSET} mg_ext;
              eval ${_UNSET} mg_f;
              eval ${_UNSET} mg_list;
              eval ${_UNSET} mg_name;
              eval ${_UNSET} mg_s;
              eval ${_UNSET} mg_sec;
              eval "${return_ok}";
            fi;
            ;;
          esac;
        done;			# for f
      else			# mg_ext is not empty
        for f
        do
          mg_f="$f";
### man_get()
          case "${mg_f}" in
*/man"${mg_sec}"/"${mg_name}"."${mg_sec}""${mg_ext}"|\
*/man"${mg_sec}"/"${mg_name}"."${mg_sec}""${mg_ext}".*)
            if obj mg_f is_file
            then
              obj mg_f to_tmp && \
                register_title "${mg_name}(${mg_sec}${mg_ext})";
              eval ${_UNSET} mg_ext;
              eval ${_UNSET} mg_f;
              eval ${_UNSET} mg_list;
              eval ${_UNSET} mg_name;
              eval ${_UNSET} mg_s;
              eval ${_UNSET} mg_sec;
              eval "${return_ok}";
            fi;
            ;;
          esac;
        done;			# for f
        for f
        do
          mg_f="$f";
### man_get()
          case "${mg_f}" in
          */man"${mg_sec}"/"${mg_name}"."${mg_sec}""${mg_ext}"*)
            if obj mg_f is_file
            then
              obj mg_f to_tmp && \
                register_title "${mg_name}(${mg_sec}${mg_ext})";
              eval ${_UNSET} mg_ext;
              eval ${_UNSET} mg_f;
              eval ${_UNSET} mg_list;
              eval ${_UNSET} mg_name;
              eval ${_UNSET} mg_s;
              eval ${_UNSET} mg_sec;
              eval "${return_ok}";
            fi;
            ;;
          esac;
        done;			# for f
      fi;
    fi;				# $mg_sec
  fi;				# $_MAN_ALL

  eval ${_UNSET} mg_ext;
  eval ${_UNSET} mg_f;
  eval ${_UNSET} mg_list;
  eval ${_UNSET} mg_name;
  eval ${_UNSET} mg_sec;
  eval ${_UNSET} mg_s;
  eval "${return_ok}";
} # man_get()


########################################################################
# man_is_man (<man-name> [<section> [<extension>]])
#
# Test whether <man-name> exists as man page.
#
# Globals: in: $_TMP_MAN, $_MAN_SEC_CHARS, $_TMP_DIR, $_MAN_EXT,
#              $_MAN_AUTO_SEC_CHARS
#         out: $_TMP_MANSPEC
#
# Arguments: 1, 2, or 3
#
# Variable prefix: mim
#
man_is_man()
{
  func_check man_is_man '>=' 1 "$@";
  if is_empty "$1"
  then
    _TMP_MANSPEC='';
    eval "${return_no}";
  fi;
  if obj _TMP_DIR is_empty
  then
    error 'man_is_man(): main_init() must be run first.';
  fi;
  if obj _MAN_IS_SETUP is_not_yes
  then
    error 'man_is_man(): man_setup() must be run first.';
  fi;
  mim_sec="$2";
  if is_empty "$2"
  then
    mim_sec="${_MAN_SEC_CHARS}";
  fi;
  if is_empty "$3"
  then
    mim_ext="${_MAN_EXT}";
  else
    mim_ext="$3";
  fi;
  _TMP_MANSPEC="${_TMP_DIR}/,man:$1:${mim_sec}${mim_ext}";
### man_is_man()
  if obj _TMP_MANSPEC is_not_file
  then
    if obj mim_sec is_empty
    then
      m="${_MAN_AUTO_SEC_CHARS}";
      eval grep "'/man$m/$1\.$m${mim_ext}'" \
           "${_TMP_MAN}" > "${_TMP_MANSPEC}";
    else
      eval grep "'/man${mim_sec}/$1\.${mim_sec}${mim_ext}'" \
           "${_TMP_MAN}" > "${_TMP_MANSPEC}";
    fi;
  fi;
  eval ${_UNSET} mim_ext;
  eval ${_UNSET} mim_sec;
  if obj _TMP_MANSPEC is_empty_file
  then
    rm_file_with_debug "${_TMP_MANSPEC}";
    eval "${return_no}";
  else
    eval "${return_yes}";
  fi;
} # man_is_man()


########################################################################
# man_setup ()
#
# Setup the variables $_MAN_* needed for man page searching.
#
# Globals:
#   in:     $_OPT_*, $_MANOPT_*, $LANG, $LC_MESSAGES, $LC_ALL,
#           $MANPATH, $MANSEC, $PAGER, $SYSTEM, $MANOPT.
#   out:    $_MAN_PATH, $_MAN_LANG, $_MAN_SYS, $_MAN_LANG, $_MAN_LANG2,
#           $_MAN_SEC, $_MAN_ALL, $_TMP_MAN
#   in/out: $_MAN_ENABLE
#
# The precedence for the variables related to `man' is that of GNU
# `man', i.e.
#
# $LANG; overridden by
# $LC_MESSAGES; overridden by
# $LC_ALL; this has the same precedence as
# $MANPATH, $MANSEC, $PAGER, $SYSTEM; overridden by
# $MANOPT; overridden by
# the groffer command line options.
#
# $MANROFFSEQ is ignored because grog determines the preprocessors.
#
# Variable prefix: ms
#
man_setup()
{
  func_check man_setup '=' 0 "$@";

  if obj _MAN_IS_SETUP is_yes
  then
    eval "${return_ok}";
  fi;
  _MAN_IS_SETUP='yes';

  if obj _MAN_ENABLE is_not_yes
  then
    eval "${return_ok}";
  fi;

  # determine basic path for man pages
  obj_from_output ms_path \
    get_first_essential "${_OPT_MANPATH}" "${_MANOPT_PATH}" "${MANPATH}";
  if obj ms_path is_empty && is_prog 'manpath'
  then
    obj_from_output ms_path manpath 2>${_NULL_DEV}; # not always available
  fi;
  if obj ms_path is_empty
  then
    manpath_set_from_path;
  else
    obj_from_output _MAN_PATH path_list "${ms_path}";
  fi;
  if obj _MAN_PATH is_empty
  then
    _MAN_ENABLE="no";
    echo2 "man_setup(): man path is empty";
    eval ${_UNSET} ms_path;
    eval "${return_ok}";
  fi;
  obj_from_output _MAN_PATH list_uniq _MAN_PATH;
### man_setup()

  if obj _MAN_ALL is_not_yes
  then
    if obj _OPT_ALL is_yes || obj _MANOPT_ALL is_yes
    then
      _MAN_ALL='yes';
    else
      _MAN_ALL='no';
    fi;
  fi;

  ms_sys="$(get_first_essential \
              "${_OPT_SYSTEMS}" "${_MANOPT_SYS}" "${SYSTEM}")";
  if obj ms_sys is_not_empty
  then
    obj_from_output _MAN_SYS list_from_split "${ms_sys}" ',';
  fi;

  obj_from_output ms_lang get_first_essential \
           "${_OPT_LANG}" "${LC_ALL}" "${LC_MESSAGES}" "${LANG}";
  case "${ms_lang}" in
    C|POSIX)
      _MAN_LANG="";
      _MAN_LANG2="";
      ;;
    ?)
      _MAN_LANG="${ms_lang}";
      _MAN_LANG2="";
      ;;
    ??)
      _MAN_LANG="${ms_lang}";
      _MAN_LANG2="${ms_lang}";
      ;;
### man_setup()
    *)
      _MAN_LANG="${ms_lang}";
      # get first two characters of $ms_lang
      _MAN_LANG2="$(echo1 "${ms_lang}" | sed 's/^\(..\).*$/\1/')";
      exit_test;
      ;;
  esac;
  # from now on, use only $_MAN_LANG*, forget about $_OPT_LANG, $LC_*.

  manpath_add_lang_sys;
  obj_from_output _MAN_PATH list_uniq _MAN_PATH;

  obj_from_output _MAN_SEC get_first_essential \
    "${_OPT_SECTIONS}" "${_MANOPT_SEC}" "${MANSEC}";
  _MAN_SEC_LIST="";
  _MAN_SEC_CHARS="";
  case "${_MAN_SEC}" in
  *:*)
    eval set x "$(list_from_split "${_MAN_SEC}" :)";
    shift;
    for s
    do
      if list_has _MAN_AUTO_SEC_LIST "$s"
      then
        list_append _MAN_SEC_LIST "$s";
        _MAN_SEC_CHARS="${_MAN_SEC_CHARS}$s";
      fi;
    done
    if obj _MAN_SEC_CHARS is_not_empty
    then
      _MAN_SEC_CHARS="[${_MAN_SEC_CHARS}]";
    fi;
    ;;
  *)
    if list_has _MAN_AUTO_SEC_LIST "${_MAN_SEC}"
    then
      list_append _MAN_SEC_LIST "${_MAN_SEC}";
      _MAN_SEC_CHARS="[${_MAN_SEC}]";
    fi;
    ;;
  esac;

### man_setup()
  obj_from_output _MAN_EXT get_first_essential \
    "${_OPT_EXTENSION}" "${_MANOPT_EXTENSION}" "${EXTENSION}";

  _TMP_MAN="$(tmp_create man)";

  eval set x "${_MAN_PATH}";
  shift;
  if is_not_equal "$#" 0
  then
    for i
    do
      for j in "$i"/man*
      do
        if obj j is_dir
	then
          find "$j" >>"${_TMP_MAN}";
        fi;
      done
    done;
  fi;

  eval ${_UNSET} ms_lang;
  eval ${_UNSET} ms_list;
  eval ${_UNSET} ms_path;
  eval ${_UNSET} ms_sys;
  eval "${return_ok}";
} # man_setup()


########################################################################
landmark '8: manpath_*()';
########################################################################

########################################################################
# manpath_add_lang_sys ()
#
# Add language and operating system specific directories to man path.
#
# Arguments : 0
# Output    : none
# Globals:
#   in:     $_MAN_SYS: a list of names of operating systems.
#           $_MAN_LANG and $_MAN_LANG2: each a single name
#   in/out: $_MAN_PATH: list of directories which shall have the `man?'
#             subdirectories.
#
# Variable prefix: mals
#
manpath_add_lang_sys()
{
  func_check manpath_add_lang_sys '=' 0 "$@";
  if obj _MAN_PATH is_empty
  then
    eval "${return_ok}";
  fi;
  if obj _MAN_SYS is_empty
  then
    mals_mp="${_MAN_PATH}";
  else
    mals_mp='';
    eval set x "${_MAN_SYS}";
    shift;
    for s
    do
      _manpath_add_sys "$s";
    done;
  fi;

  if obj mals_mp is_not_empty
  then
    mals_lang_path='';
    if is_equal "$_MAN_LANG" "$_MAN_LANG2"
    then
      mals_man_lang2='';
    else
      mals_man_lang2="${_MAN_LANG2}";
    fi;
    for i in "${_MAN_LANG}" "${mals_man_lang2}"
    do
      if obj i is_empty
      then
        continue;
      fi;
### manpath_add_lang_sys()
      mals_lang="$i";
      eval set x "${mals_mp}";
      shift;
      for p
      do
        obj_from_output mals_dir dir_name_append "${p}" "${mals_lang}";
        if obj mals_dir is_dir
        then
          list_append mals_lang_path "${mals_dir}";
        fi;
      done;
    done;
    obj_from_output mals_mp lists_combine mals_lang_path mals_mp;
  fi;

  _MAN_PATH="${mals_mp}";
  eval ${_UNSET} mals_dir;
  eval ${_UNSET} mals_lang;
  eval ${_UNSET} mals_lang_path;
  eval ${_UNSET} mals_man_lang2;
  eval ${_UNSET} mals_mp;
  eval "${return_ok}";
} # manpath_add_lang_sys()


# _manpath_add_sys (<system>)
#
# Append the existing subdirectories <system> of man path directories to
# the list $mals_mp.
#
# Local function to manpath_add_lang_sys().
#
# Argument: 1, a operating system name (for appending to a man path
#              directory)
#
# Globals in:     $_MAN_PATH
# Globals in/out: $mals_mp
#
# Variable prefix: _mas
#
_manpath_add_sys()
{
  func_check _manpath_add_sys '=' 1 "$@";
  case "$1" in
  '')
    :;
    ;;
  man)
    obj_from_output mals_mp lists_combine mals_mp _MAN_PATH;
    ;;
  *)
    _mas_sys="$1";
    eval set x "${_MAN_PATH}";
    shift;
    for p
    do
      obj_from_output _mas_dir dir_name_append "${p}" "${_mas_sys}";
      if obj _mas_dir is_dir
      then
        list_append mals_mp "${_mas_dir}";
      fi;
    done;
    ;;
  esac;
  eval ${_UNSET} _mas_dir;
  eval ${_UNSET} _mas_sys;
  eval "${return_ok}";
} # _manpath_add_sys() of manpath_add_lang_sys()


########################################################################
# manpath_set_from_path ()
#
# Determine basic search path for man pages from $PATH.
#
# Return:    `0' if a valid man path was retrieved.
# Output:    none
# Globals:
#   in:  $PATH
#   out: $_MAN_PATH
#
# Variable prefix: msfp
#
manpath_set_from_path()
{
  func_check manpath_set_from_path '=' 0 "$@";

  msfp_manpath='';

  # get a basic man path from $PATH
  if obj PATH is_not_empty
  then
    # delete the final `/bin' part
    p="$(echo1 "${PATH}" | sed 's|//*bin/*:|:|g')";
    obj_from_output msfp_list path_list "$p";
    # append some default directories
    for b in /usr/local /usr/local /usr /usr \
           /usr/X11R6 /usr/openwin \
           /opt /opt/gnome /opt/kde
    do
      msfp_base="$b";
      if list_has_not msfp_list "${msfp_base}" && obj msfp_base is_dir
      then
        list_append msfp_list "${msfp_base}";
      fi;
    done;
    eval set x "${msfp_list}";
    shift;
    for d
    do
      # including empty for former `/bin'.
      msfp_base="$d";
      for e in /share/man /share/MAN /man /MAN
      do
        msfp_mandir="${msfp_base}$e";
        if obj msfp_mandir is_dir
        then
          list_append msfp_manpath "${msfp_mandir}";
        fi;
      done;
    done;
  fi;

  _MAN_PATH="${msfp_manpath}";
  eval ${_UNSET} msfp_base;
  eval ${_UNSET} msfp_list;
  eval ${_UNSET} msfp_mandir;
  eval ${_UNSET} msfp_manpath;
  eval "${return_ok}";
} # manpath_set_from_path()


########################################################################
landmark '9: obj_*()';
########################################################################

########################################################################
# obj (<object> <call_name> <arg>...)
#
# This works like a method (object function) call for an object.
# Run "<call_name> $<object> <arg> ...".
#
# The first argument represents an object name whose data is given as
# first argument to <call_name>().
#
# Argument: >=2
#           <object>: variable name
#           <call_name>: a program or function name
#
# Variable prefix: o
#
obj()
{
  func_check obj '>=' 2 "$@";
  eval o_arg1='"${'$1'}"';
  if is_empty "$2"
  then
    error "obj(): function name is empty."
  else
    o_func="$2";
  fi;
  shift;
  shift;
  eval "${o_func}"' "${o_arg1}" "$@"';
  n="$?";
  eval ${_UNSET} o_arg1;
  eval ${_UNSET} o_func;
  eval "${return_var} $n";
} # obj()


########################################################################
# obj_data (<object>)
#
# Print the data of <object>, i.e. the content of $<object>.
# For possible later extensions.
#
# Arguments: 1
#            <object>: a variable name
# Output:    the data of <object>
#
# Variable prefix: od
#
obj_data()
{
  func_check obj_data '=' 1 "$@";
  if is_empty "$1"
  then
    error "obj_data(): object name is empty."
  fi;
  eval od_res='"${'"$1"'}"';
  obj od_res echo1;
  eval ${_UNSET} od_res;
  eval "${return_ok}";
} # obj_data()


########################################################################
# obj_from_output (<object> <call_name> <arg>...)
#
# Run '$<object>="$(<call_name> <arg>...)"' to set the result of a
# function call to a global variable.  Variables are not stored.
#
# Arguments: >=2
#            <object>: a variable name
#            <call_name>: the name of a function or program
#            <arg>: optional argument to <call_name>
# Output:    none
#
# Variable prefix: ofo
#
obj_from_output()
{
  func_check obj_from_output '>=' 2 "$@";
  if is_empty "$1"
  then
    error "obj_from_output(): variable name is empty.";
  fi;
  if is_empty "$2"
  then
    error "obj_from_output(): function name is empty."
  fi;
  ofo_result_name="$1";
  shift;
  ofo_return=0;
  if is_equal "$#" 0
  then
    eval "${ofo_result_name}"'=""';
  else
    ofo_list='';
    for i
    do
      list_append ofo_list "$i";
    done;
    eval "${ofo_result_name}"'="$('"${ofo_list}"')"';
    ofo_return="$?";
    exit_test;
  fi;
  r="${ofo_return}";
  eval ${_UNSET} ofo_list;
  eval ${_UNSET} ofo_return;
  eval ${_UNSET} ofo_result_name;
  eval "${return_var} $r";
} # obj_from_output()


########################################################################
# obj_set (<object> <data>)
#
# Set the data of <object>, i.e. call "$<object>=<data>".
#
# Arguments: 2
#            <object>: a variable name
#            <data>: a string
# Output::   none
#
obj_set()
{
  func_check obj_set '=' 2 "$@";
  if is_empty "$1"
  then
    error "obj_set(): object name is empty."
  fi;
  eval "$1"='"$2"';
  eval "${return_ok}";
} # obj_set()


########################################################################
# path_chop (<path>)
#
# Remove unnecessary colons from path.
#
# Argument: 1, a colon separated path.
# Output:   path without leading, double, or trailing colons.
#
path_chop()
{
  func_check path_chop = 1 "$@";

  # replace multiple colons by a single colon `:'
  # remove leading and trailing colons
  echo1 "$1" | sed '
s/^:*//
s/:::*/:/g
s/:*$//
';
  eval "${return_ok}";
} # path_chop()


########################################################################
# path_clean (<path>)
#
# Remove non-existing directories from a colon-separated list.
#
# Argument: 1, a colon separated path.
# Output:   colon-separated list of existing directories.
#
# Variable prefix: pc
#
path_clean()
{
  func_check path_clean = 1 "$@";
  if is_not_equal "$#" 1
  then
    error 'path_clean() needs 1 argument.';
  fi;
  pc_arg="$1";
  eval set x "$(path_list "${pc_arg}")";
  exit_test;
  shift;
  pc_res="";
  for i
  do
    pc_i="$i";
    if obj pc_i is_not_empty \
       && obj pc_res path_not_contains "${pc_i}" \
       && obj pc_i is_dir
    then
      case "${pc_i}" in
      ?*/)
        pc_res="${pc_res}:$(dir_name_chop "${pc_i}")";
        exit_test;
        ;;
      *)
        pc_res="${pc_res}:${pc_i}";
        ;;
      esac;
    fi;
  done;
  path_chop "${pc_res}";
  eval ${_UNSET} pc_arg;
  eval ${_UNSET} pc_i;
  eval ${_UNSET} pc_res;
  eval "${return_ok}";
} # path_clean()


########################################################################
# path_contains (<path> <dir>)
#
# Test whether <dir> is contained in <path>, a list separated by `:'.
#
# Arguments : 2
# Return    : `0' if arg2 is substring of arg1, `1' otherwise.
#
path_contains()
{
  func_check path_contains = 2 "$@";
  case ":$1:" in
    *:${2}:*)
      eval "${return_yes}";
      ;;
    *)
      eval "${return_no}";
      ;;
  esac;
  eval "${return_ok}";
} # path_contains()


########################################################################
# path_not_contains (<path> <dir>)
#
# Test whether <dir> is not contained in colon separated <path>.
#
# Arguments : 2
#
path_not_contains()
{
  func_check path_not_contains = 2 "$@";
  if path_contains "$1" "$2"
  then
    eval "${return_no}";
  else
    eval "${return_yes}";
  fi;
  eval "${return_ok}";
} # path_not_contains()


########################################################################
# path_list (<path>)
#
# From a `:' separated path generate a list with unique elements.
#
# Arguments: 1: a colon-separated path
# Output:    the resulting list, process it with `eval set'
#
# Variable prefix: pl
#
path_list()
{
  func_check path_list = 1 "$@";
  eval set x "$(list_from_split "$1" '\:')";
  shift;
  pl_list='';
  for e
  do
    pl_elt="$e";
    if list_has pl_list "${pl_elt}"
    then
      continue;
    else
      list_append pl_list "${pl_elt}";
    fi;
  done;
  obj pl_list echo1;
  eval ${_UNSET} pl_elt;
  eval ${_UNSET} pl_list;
  eval "${return_ok}";
} # path_list()


########################################################################
landmark '10: register_*()';
########################################################################

########################################################################
# register_file (<filename>)
#
# Write a found file and register the title element.
#
# Arguments: 1: a file name
# Output: none
#
register_file()
{
  func_check register_file = 1 "$@";
  if is_empty "$1"
  then
    error 'register_file(): file name is empty';
  fi;
  if is_equal "$1" '-'
  then
    to_tmp "${_TMP_STDIN}" && register_title 'stdin';
  else
    to_tmp "$1" && register_title "$1";
    exit_test;
  fi;
  eval "${return_ok}";
} # register_file()


########################################################################
# register_title (<filespec>)
#
# Create title element from <filespec> and append to $_REG_TITLE_LIST.
# Basename is created.
#
# Globals: $_REG_TITLE_LIST (rw)
#
# Variable prefix: rt
#
register_title()
{
  func_check register_title '=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_ok}";
  fi;

  if obj _DEBUG_PRINT_FILENAMES is_yes
  then
    if is_equal "$1" 'stdin'
    then
      echo2 "file: standard input";
    else
      if obj _FILESPEC_IS_MAN is_yes
      then
        echo2 "file title: $1";
      else
        echo2 "file: $1";
      fi;
    fi;
  fi;

  case "${_REG_TITLE_LIST}" in
  *\ *\ *\ *)
    eval "${return_ok}";
    ;;
  esac;

  # remove directory part
  obj_from_output rt_title base_name "$1";
  # replace space characters by `_'
  rt_title="$(echo1 "${rt_title}" | sed 's/[ 	]/_/g')";
  # remove extension `.bz2'
  rt_title="$(echo1 "${rt_title}" | sed 's/\.bz2$//')";
  # remove extension `.gz'
  rt_title="$(echo1 "${rt_title}" | sed 's/\.gz$//')";
  # remove extension `.Z'
  rt_title="$(echo1 "${rt_title}" | sed 's/\.Z$//')";

  if obj rt_title is_empty
  then
    eval ${_UNSET} rt_title;
    eval "${return_ok}";
  fi;
  list_append _REG_TITLE_LIST "${rt_title}";
  eval ${_UNSET} rt_title;
  eval "${return_ok}";
} # register_title()


########################################################################
# reset ()
#
# Reset the variables that can be affected by options to their default.
#
#
# Defined in section `Preset' after the rudimentary shell tests.


########################################################################
# rm_file (<file_name>)
#
# Remove file.
#
rm_file()
{
  func_check rm_file '=' 1 "$@";
  if is_file "$1"
  then
    rm -f "$1" >${_NULL_DEV} 2>&1;
  fi;
  if is_existing "$1"
  then
    eval "${return_bad}";
  else
    eval "${return_good}";
  fi;
} # rm_file()


########################################################################
# rm_file_with_debug (<file_name>)
#
# Remove file if $_DEBUG_KEEP_FILES allows it.
#
# Globals: $_DEBUG_KEEP_FILES
#
rm_file_with_debug()
{
  func_check rm_file_with_debug '=' 1 "$@";
  if obj _DEBUG_KEEP_FILES is_not_yes
  then
    if is_file "$1"
    then
      rm -f "$1" >${_NULL_DEV} 2>&1;
    fi;
  fi;
  if is_existing "$1"
  then
    eval "${return_bad}";
  else
    eval "${return_good}";
  fi;
} # rm_file_with_debug()


########################################################################
# rm_tree (<dir_name>)
#
# Remove a file or a complete directory tree.
#
# Globals: $_DEBUG_KEEP_FILES
#
rm_tree()
{
  func_check rm_tree '=' 1 "$@";
  if is_existing "$1"
  then
    rm -f -r "$1" >${_NULL_DEV} 2>&1;
  fi;
  if is_existing "$1"
  then
    eval "${return_bad}";
  else
    eval "${return_good}";
  fi;
} # rm_tree()


########################################################################
# save_stdin ()
#
# Store standard input to temporary file (with decompression).
#
# Variable prefix: ss
#
if obj _HAS_COMPRESSION is_yes
then
  save_stdin()
  {
    func_check save_stdin '=' 0 "$@";
    ss_f="${_TMP_DIR}"/INPUT;
    cat >"${ss_f}";
    cat_z "${ss_f}" >"${_TMP_STDIN}";
    rm_file "${ss_f}";
    eval ${_UNSET} ss_f;
    eval "${return_ok}";
  } # save_stdin()
else				# no compression
  save_stdin()
  {
    func_check save_stdin '=' 0 "$@";
    cat >"${_TMP_STDIN}";
    eval "${return_ok}";
  } # save_stdin()
fi;


########################################################################
# special_filespec ()
#
# Handle special modes like whatis and apropos.  Run their filespec
# functions if suitable.
#
# Globals:  in: $_OPT_APROPOS, $_OPT_WHATIS, $_SPECIAL_SETUP
#          out: $_SPECIAL_FILESPEC (internal)
#
special_filespec()
{
  func_check special_filespec '=' 0 "$@";
  if obj _OPT_APROPOS is_not_yes && obj _OPT_WHATIS is_not_yes
  then
    eval "${return_bad}";
  fi;
  if obj _OPT_APROPOS is_yes && obj _OPT_WHATIS is_yes
  then
    error \
      'special_filespec(): $_OPT_APROPOS and $_OPT_WHATIS are both "yes"';
  fi;
  if obj _SPECIAL_SETUP is_not_yes
  then
    error 'special_filespec(): setup for apropos or whatis must be run first.';
  fi;
  if apropos_filespec || whatis_filespec;
  then
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # special_filespec()


########################################################################
# special_setup ()
#
# Handle special modes like whatis and apropos.  Run their setup
# functions if suitable.
#
special_setup()
{
  func_check special_setup '=' 0 "$@";
  if obj _OPT_APROPOS is_yes && obj _OPT_WHATIS is_yes
  then
    error \
      'special_setup(): $_OPT_APROPOS and $_OPT_WHATIS are both "yes"';
  fi;
  if apropos_setup || whatis_setup
  then
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # special_setup()


########################################################################
landmark '11: stack_*()';
########################################################################

########################################################################
# string_contains (<string> <part>)
#
# Test whether <part> is contained in <string>.
#
# Arguments : 2 text arguments.
# Return    : `0' if arg2 is substring of arg1, `1' otherwise.
#
string_contains()
{
  func_check string_contains '=' 2 "$@";
  case "$1" in
    *${2}*)
      eval "${return_yes}";
      ;;
    *)
      eval "${return_no}";
      ;;
  esac;
  eval "${return_ok}";
} # string_contains()


########################################################################
# string_not_contains (<string> <part>)
#
# Test whether <part> is not substring of <string>.
#
# Arguments : 2 text arguments.
# Return    : `0' if arg2 is substring of arg1, `1' otherwise.
#
string_not_contains()
{
  func_check string_not_contains '=' 2 "$@";
  if string_contains "$1" "$2"
  then
    eval "${return_no}";
  else
    eval "${return_yes}";
  fi;
  eval "${return_ok}";
} # string_not_contains()


########################################################################
landmark '12: tmp_*()';
########################################################################

########################################################################
# tmp_cat ()
#
# Output the temporary cat file (the concatenation of all input).
#
tmp_cat()
{
  func_check tmp_cat '=' 0 "$@";
  cat "${_TMP_CAT}";
  eval "${return_var}" "$?";
} # tmp_cat()


########################################################################
# tmp_create (<suffix>?)
#
# Create temporary file.  The generated name is `,' followed by
# <suffix>.
#
# Argument: 0 or 1
#
# Globals: $_TMP_DIR
#
# Output : name of created file
#
# Variable prefix: tc
#
tmp_create()
{
  func_check tmp_create '<=' 1 "$@";
  if obj _TMP_DIR is_empty || obj _TMP_DIR is_not_dir
  then
    error 'tmp_create(): there is no temporary directory.';
  else
    # the output file does not have `,' as first character, so these are
    # different names from the output file.
    tc_tmp="${_TMP_DIR}/,$1";
    obj tc_tmp rm_file;
    : >"${tc_tmp}"
    obj tc_tmp echo1;
  fi;
  eval ${_UNSET} tc_tmp;
  eval "${return_ok}";
} # tmp_create()


########################################################################
# to_tmp (<filename>)
#
# Print file (decompressed) to the temporary cat file.
#
# Variable prefix: tt
#
to_tmp()
{
  func_check to_tmp '=' 1 "$@";
  if obj _TMP_CAT is_empty
  then
    error 'to_tmp(): $_TMP_CAT is not yet set';
  fi;
  tt_1="$1";
  tt_so_nr=0;			# number for temporary `,so,*,*'
  if is_file "${tt_1}"
  then
    tt_dir="$(dir_name "${tt_1}")";
    if obj _OPT_WHATIS is_yes
    then
      whatis_filename "${tt_1}" >>"${_TMP_CAT}";
    else
      _FILE_NR="$(expr ${_FILE_NR} + 1)";
      tt_file="${_TMP_DIR}/,file${_FILE_NR}";
      if obj _FILESPEC_IS_MAN is_yes
      then
        if obj _DEBUG_PRINT_FILENAMES is_yes
        then
          echo2 "file: ${tt_1}";
        fi;
        tt_tmp="${_TMP_DIR}/,tmp";
        cat_z "${tt_1}" >"${tt_file}";
        grep '^\.[ 	]*so[ 	]' "${tt_file}" |
	  sed 's/^\.[ 	]*so[ 	]*//' >"${tt_tmp}";
        list_from_file tt_list "${tt_tmp}";
        eval set x ${tt_list};
        shift;
        for i
        do
          tt_i="$i";
          tt_so_nr="$(expr ${tt_so_nr} + 1)";
          tt_sofile="${_TMP_DIR}/,so${_FILE_NR}_${tt_so_nr}";
          tt_sofiles="${tt_sofiles} ${tt_sofile}";
          _do_man_so "${tt_i}";
        done;
        rm_file "${tt_tmp}";
        mv "${tt_file}" "${tt_tmp}";
        cat "${tt_tmp}" | soelim -I "${tt_dir}" ${_SOELIM_R} >"${tt_file}";
        for f in ${tt_sofiles}
        do
          rm_file_with_debug $f;
        done;
        rm_file "${tt_tmp}";
      else			# $_FILESPEC_IS_MAN ist not yes
        cat_z "${tt_1}" | soelim -I "${tt_dir}" ${_SOELIM_R} >"${tt_file}";
      fi;
### to_tmp()
      obj_from_output tt_grog grog "${tt_file}";
      if is_not_equal "$?" 0
      then
        exit "${_ERROR}";
      fi;
      echo2 "grog output: ${tt_grog}";
      case " ${tt_grog} " in
      *\ -m*)
        eval set x "$(echo1 " ${tt_grog} " | sed '
s/'"${_TAB}"'/ /g
s/  */ /g
s/ -m / -m/g
s/ -mm\([^ ]\)/ -m\1/g
')";
        shift;
        for i
        do
          tt_i="$i";
          case "${tt_i}" in
          -m*)
            if list_has _MACRO_PACKAGES "${tt_i}"
            then
              case "${_MACRO_PKG}" in
              '')
                _MACRO_PKG="${tt_i}";
                ;;
              ${tt_i})
                :;
                ;;
              -m*)
                echo2 "Ignore ${tt_1} because it needs ${tt_i} instead "\
"of ${_MACRO_PKG}."
                rm_file_with_debug "${tt_file}";
                eval ${_UNSET} tt_1;
                eval ${_UNSET} tt_dir;
                eval ${_UNSET} tt_file;
                eval ${_UNSET} tt_grog;
                eval ${_UNSET} tt_i;
                eval ${_UNSET} tt_so_nr;
                eval ${_UNSET} tt_sofile;
                eval ${_UNSET} tt_sofiles;
                eval ${_UNSET} tt_sofound;
                eval ${_UNSET} tt_list;
                eval ${_UNSET} tt_tmp;
                eval "${return_bad}";
                ;;
### to_tmp()
              *)
                error \
'to_tmp(): $_MACRO_PKG does not start with -m: '"${_MACRO_PKG}";
                ;;
              esac;
            fi;
            ;;
          esac;
        done;
        ;;
      esac;
      cat "${tt_file}" >>"${_TMP_CAT}";
      rm_file_with_debug "${tt_file}";
    fi;
  else
    error "to_tmp(): could not read file \`${tt_1}'.";
  fi;
  eval ${_UNSET} tt_1;
  eval ${_UNSET} tt_dir;
  eval ${_UNSET} tt_file;
  eval ${_UNSET} tt_grog;
  eval ${_UNSET} tt_i;
  eval ${_UNSET} tt_so_nr;
  eval ${_UNSET} tt_sofile;
  eval ${_UNSET} tt_sofiles;
  eval ${_UNSET} tt_sofound;
  eval ${_UNSET} tt_list;
  eval ${_UNSET} tt_tmp;
  eval "${return_ok}";
} # to_tmp()


#############
# _do_man_so (<so_arg>)
#
# Handle single .so file name for man pages.
#
# Local function to to_tmp().
#
# Globals from to_tmp(): $tt_tmp, $tt_sofile, $tt_file
# Globals: $_TMP_MAN
#
# Variable prefix: dms
#
_do_man_so() {
  func_check _do_man_so '=' 1 "$@";
  _dms_so="$1";			# evt. with `\ '
  _dms_soname="$(echo $1 | sed 's/\\[ 	]/ /g')"; # without `\ '
  case "${_dms_soname}" in
  /*)				# absolute path
    if test -f "${_dms_soname}"
    then
      eval "${return_ok}";
    fi;
    if test -f "${_dms_soname}"'.gz'
    then
      _dms_sofound="${_dms_soname}"'.gz';
    elif test -f "${_dms_soname}"'.Z'
    then
      _dms_sofound="${_dms_soname}"'.Z';
    elif test -f "${_dms_soname}"'.bz2'
    then
      _dms_sofound="${_dms_soname}"'.bz2';
    else
      eval ${_UNSET} _dms_so;
      eval ${_UNSET} _dms_soname;
      eval "${return_ok}";
    fi;
    ;;
### _do_man_so() of to_tmp()
  *)				# relative to man path
    eval grep "'/${_dms_soname}\$'" "${_TMP_MAN}" >"${tt_tmp}";
    if is_empty_file "${tt_tmp}"
    then
      eval grep "'/${_dms_soname}.gz\$'" "${_TMP_MAN}" >"${tt_tmp}";
      if is_empty_file "${tt_tmp}"
      then
        eval grep "'/${_dms_soname}.Z\$'" "${_TMP_MAN}" >"${tt_tmp}";
        if is_empty_file "${tt_tmp}"
        then
          eval grep "'/${_dms_soname}.bz2\$'" "${_TMP_MAN}" >"${tt_tmp}";
        fi;
      fi;
    fi;
    if is_empty_file "${tt_tmp}"
    then
      eval "${return_ok}";
    fi;
    _dms_done='no';
    list_from_file _dms_list "${tt_tmp}";
    eval set x ${_dms_list};
    shift;
    for i
    do
      _dms_sofound="$i";
      if obj _dms_sofound is_empty
      then
        continue;
      fi;
      _dms_done='yes';
      break;
    done;
### _do_man_so() of to_tmp()
    if obj _dms_done is_not_yes
    then
      eval ${_UNSET} _dms_done;
      eval ${_UNSET} _dms_sofound;
      eval "${return_ok}";
    fi;
    ;;
  esac;
  if obj _DEBUG_PRINT_FILENAMES is_yes
  then
    echo2 "file from .so: ${_dms_so}";
  fi;
  cat_z "${_dms_sofound}" >"${tt_sofile}";
  _dms_esc="$(echo ${_dms_so} | sed 's/\\/\\\\/g')";
  cat "${tt_file}" | eval sed \
"'s#^\\.[ 	]*so[ 	]*\(${_dms_so}\|${_dms_esc}\|${_dms_soname}\)[ 	]*\$'"\
"'#.so ${tt_sofile}#'" \
    >"${tt_tmp}";
  rm_file "${tt_file}";
  mv "${tt_tmp}" "${tt_file}";
  eval ${_UNSET} _dms_done;
  eval ${_UNSET} _dms_esc;
  eval ${_UNSET} _dms_so;
  eval ${_UNSET} _dms_sofound;
  eval ${_UNSET} _dms_soname;
  eval "${return_ok}";
} # _do_man_so() of to_tmp()


########################################################################
# to_tmp_line (<text>...)
#
# Print single line with <text> to the temporary cat file.
#
to_tmp_line()
{
  func_check to_tmp_line '>=' 1 "$@";
  if obj _TMP_CAT is_empty
  then
    error 'to_tmp_line(): $_TMP_CAT is not yet set';
  fi;
  echo1 "$*" >>"${_TMP_CAT}";
  eval "${return_ok}";
} # to_tmp_line()


########################################################################
# trap_set
#
# Call function on signal 0.
#
trap_set()
{
  func_check trap_set '=' 0 "$@";
  trap 'clean_up' 0 2>${_NULL_DEV} || :;
  eval "${return_ok}";
} # trap_set()


########################################################################
# trap_unset ()
#
# Disable trap on signal 0.
#
trap_unset()
{
  func_check trap_unset '=' 0 "$@";
  trap '' 0 2>${_NULL_DEV} || :;
  eval "${return_ok}";
} # trap_unset()


########################################################################
# usage ()
#
# Print usage information to standard output; for groffer option --help.
#
usage()
{
  func_check usage = 0 "$@";
  echo;
  version;
  cat <<EOF

Usage: groffer [option]... [filespec]...

Display roff files, standard input, and/or Unix manual pages with a X
Window viewer or in several text modes.  All input is decompressed
on-the-fly with all formats that gzip can handle.

"filespec" is one of
  "filename"       name of a readable file
  "-"              for standard input
  "man:name(n)"    man page "name" in section "n"
  "man:name.n"     man page "name" in section "n"
  "man:name"       man page "name" in first section found
  "name(n)"        man page "name" in section "n"
  "name.n"         man page "name" in section "n"
  "n name"         man page "name" in section "n"
  "name"           man page "name" in first section found
where `section' is a single character out of [1-9on], optionally followed
by some more letters that are called the `extension'.

-h --help         print this usage message.
-T --device=name  pass to groff using output device "name".
-v --version      print version information.
-V                display the groff execution pipe instead of formatting.
-X                display with "gxditview" using groff -X.
-Z --ditroff --intermediate-output
                  generate groff intermediate output without
                  post-processing and viewing, like groff -Z.
All other short options are interpreted as "groff" formatting options.

The most important groffer long options are

--apropos=name    start man's "apropos" program for "name".
--apropos-data=name
                  "apropos" for "name" in man's data sections 4, 5, 7.
--apropos-devel=name
                  "apropos" for "name" in development sections 2, 3, 9.
--apropos-progs=name
                  "apropos" for "name" in man's program sections 1, 6, 8.
--auto            choose mode automatically from the default mode list.
--default         reset all options to the default value.
--default-modes=mode1,mode2,...
                  set sequence of automatically tried modes.
--dvi             display in a viewer for TeX device independent format.
--dvi-viewer=prog choose the viewer program for dvi mode.
--groff           process like groff, disable viewing features.
--help            display this helping output.
--html            display in a web browser.
--html-viewer=program
                  choose a web browser for html mode.
--man             check file parameters first whether they are man pages.
--mode=auto|dvi|groff|html|pdf|ps|source|text|tty|www|x|X
                  choose display mode.
--no-man          disable man-page facility.
--no-special      disable --all, --apropos*, and --whatis
--pager=program   preset the paging program for tty mode.
--pdf             display in a PDF viewer.
--pdf-viewer=prog choose the viewer program for pdf mode.
--ps              display in a Postscript viewer.
--ps-viewer=prog  choose the viewer program for ps mode.
--shell=program   specify a shell under which to run groffer2.sh.
--source          output as roff source.
--text            output in a text device without a pager.
--to-stdout       output the content of the mode file without display.
--tty             display with a pager on text terminal even when in X.
--tty-viewer=prog select a pager for tty mode; same as --pager.
--whatis          display the file name and description of man pages
--www             same as --html.
--www-viewer=prog same as --html-viewer
--x --X           display with "gxditview" using an X* device.
--x-viewer=prog   choose viewer program for x mode (X mode).
--X-viewer=prog   same as "--xviewer".

The usual X Windows toolkit options transformed into GNU long options:
--background=color, --bd=size, --bg=color, --bordercolor=color,
--borderwidth=size, --bw=size, --display=Xdisplay, --fg=color,
--fn=font, --font=font, --foreground=color, --geometry=geom, --iconic,
--resolution=dpi, --rv, --title=text, --xrm=resource

Long options of GNU "man":
--all, --ascii, --ditroff, --extension=suffix, --locale=language,
--local-file=name, --location, --manpath=dir1:dir2:...,
--sections=s1:s2:..., --systems=s1,s2,..., --where, ...

Development options that are not useful for normal usage:
--debug, --debug-all, --debug-filenames, --debug-func,
--debug-not-func, --debug-grog, --debug-keep, --debug-lm,
--debug-params, --debug-shell, --debug-stacks, --debug-tmpdir,
--debug-user, --do-nothing, --print=text, --shell=prog

EOF

  eval "${return_ok}";
} # usage()


########################################################################
# version ()
#
# Print version information to standard output.
# For groffer option --version.
#
version()
{
  func_check version = 0 "$@";
  y="$(echo "${_LAST_UPDATE}" | sed 's/^.* //')";
  cat <<EOF
groffer ${_PROGRAM_VERSION} of ${_LAST_UPDATE} (shell version)
is part of groff version ${_GROFF_VERSION}.
Copyright (C) $y Free Software Foundation, Inc.
GNU groff and groffer come with ABSOLUTELY NO WARRANTY.
You may redistribute copies of groff and its subprograms
under the terms of the GNU General Public License.
EOF
  eval "${return_ok}";
} # version()


########################################################################
# warning (<string>)
#
# Print warning to stderr.
#
warning()
{
  echo2 "warning: $*";
} # warning()


########################################################################
# whatis_filename (<filename>)
#
# Interpret <filename> as a man page and display its `whatis'
# information as a fragment written in the groff language.
#
# Globals:  in: $_OPT_WHATIS, $_SPECIAL_SETUP, $_SPECIAL_FILESPEC,
#               $_FILESPEC_ARG
#
# Variable prefix: wf
#
whatis_filename()
{
  func_check whatis_filename = 1 "$@";
  if obj _OPT_WHATIS is_not_yes
  then
    error 'whatis_filename(): $_OPT_WHATIS is not yes.';
  fi;
  if obj _SPECIAL_SETUP is_not_yes
  then
    error \
      'whatis_filename(): setup for whatis whatis_setup() must be run first.';
  fi;
  if obj _SPECIAL_FILESPEC is_not_yes
  then
    error 'whatis_filename(): whatis_filespec() must be run first.';
  fi;
  wf_arg="$1";
  if obj wf_arg is_not_file
  then
    error "whatis_filename(): argument is not a readable file."
  fi;
  wf_dot='^\.'"${_SPACE_SED}"'*';
### whatis_filename()
  if obj _FILESPEC_ARG is_equal '-'
  then
    wf_arg='stdin';
  fi;
  cat <<EOF
\f[CR]${wf_arg}\f[]:
.br
EOF

  # get the parts of the file name
  wf_name="$(base_name $1)";
  wf_section="$(echo1 $1 | sed -n '
s|^.*/man\('"${_MAN_AUTO_SEC_CHARS}"'\).*$|\1|p
')";
  if obj wf_section is_not_empty
  then
    case "${wf_name}" in
    *.${wf_section}*)
      s='yes';
      ;;
    *)
      s='';
      wf_section='';
      ;;
### whatis_filename()
    esac
    if obj s is_yes
    then
      wf_name="$(echo1 ${wf_name} | sed '
s/^\(.*\)\.'${wf_section}'.*$/\1/
')";
    fi;
  fi;

  # traditional man style; grep the line containing `.TH' macro, if any
  wf_res="$(cat_z "$1" | sed '
/'"${wf_dot}"'TH /p
d
')";
  exit_test;
  if obj wf_res is_not_empty
  then				# traditional man style
    # get the first line after the first `.SH' macro, by
    # - delete up to first .SH;
    # - print all lines before the next .SH;
    # - quit.
    wf_res="$(cat_z "$1" | sed -n '
1,/'"${wf_dot}"'SH/d
/'"${wf_dot}"'SH/q
p
')";

    if obj wf_section is_not_empty
    then
      case "${wf_res}" in
      ${wf_name}${_SPACE_CASE}*-${_SPACE_CASE}*)
        s='yes';
        ;;
### whatis_filename()
      *)
        s='';
        ;;
      esac;
      if obj s is_yes
      then
        wf_res="$(obj wf_res echo1 | sed '
s/^'"${wf_name}${_SPACE_SED}"'[^-]*-'"${_SPACE_SED}"'*\(.*\)$/'"${wf_name}"' ('"${wf_section}"') \\[em] \1/
')";
      fi;
    fi;
    obj wf_res echo1;
    echo;
    eval ${_UNSET} wf_arg;
    eval ${_UNSET} wf_dot;
    eval ${_UNSET} wf_name;
    eval ${_UNSET} wf_res;
    eval ${_UNSET} wf_section;
    eval "${return_ok}";
  fi;

  # mdoc style (BSD doc); grep the line containing `.Nd' macro, if any
  wf_res="$(cat_z "$1" | sed -n '/'"${wf_dot}"'Nd /s///p')";
  exit_test;
  if obj wf_res is_not_empty
  then				# BSD doc style
    if obj wf_section is_not_empty
    then
      wf_res="$(obj wf_res echo1 | sed -n '
s/^\(.*\)$/'"${wf_name}"' ('"${wf_section}"') \\[em] \1/p
')";
    fi;
### whatis_filename()
    obj wf_res echo1;
    echo;
    eval ${_UNSET} wf_arg;
    eval ${_UNSET} wf_dot;
    eval ${_UNSET} wf_name;
    eval ${_UNSET} wf_res;
    eval ${_UNSET} wf_section;
    eval "${return_ok}";
  fi;
  echo1 'is not a man page';
  echo;
  eval ${_UNSET} wf_arg;
  eval ${_UNSET} wf_dot;
  eval ${_UNSET} wf_name;
  eval ${_UNSET} wf_res;
  eval ${_UNSET} wf_section;
  eval "${return_bad}";
} # whatis_filename()



########################################################################
# whatis_filespec ()
#
# Print the filespec name as .SH to the temporary cat file.
#
# Globals:  in: $_OPT_WHATIS, $_SPECIAL_SETUP
#          out: $_SPECIAL_FILESPEC
#
whatis_filespec()
{
  func_check whatis_filespec '=' 0 "$@";
  if obj _OPT_WHATIS is_yes
  then
    if obj _SPECIAL_SETUP is_not_yes
    then
      error 'whatis_filespec(): whatis_setup() must be run first.';
    fi;
    _SPECIAL_FILESPEC='yes';
    eval to_tmp_line \
      "'.SH $(echo1 "${_FILESPEC_ARG}" | sed 's/[^\\]-/\\-/g')'";
    exit_test;
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # whatis_filespec()


########################################################################
# whatis_setup ()
#
# Print the whatis header to the temporary cat file; this is the setup
# for whatis.
#
# Globals:  in: $_OPT_WHATIS
#          out: $_SPECIAL_SETUP
#
whatis_setup()
{
  func_check whatis_setup '=' 0 "$@";
  if obj _OPT_WHATIS is_yes
  then
    to_tmp_line '.TH GROFFER WHATIS';
    _SPECIAL_SETUP='yes';
    if obj _OPT_TITLE is_empty
    then
      _OPT_TITLE='whatis';
    fi;
    eval "${return_ok}";
  else
    eval "${return_bad}";
  fi;
} # whatis_setup()


########################################################################
# where_is_prog (<program>)
#
# Output path of a program and the given arguments if in $PATH.
#
# Arguments : 1, <program> can have spaces and arguments.
# Output    : list of 2 elements: prog name (with directory) and arguments
# Return    : `0' if arg1 is a program in $PATH, `1' otherwise.
#
# Variable prefix: wip
#
where_is_prog()
{
  func_check where_is_prog '=' 1 "$@";
  if is_empty "$1"
  then
    eval "${return_bad}";
  fi;

  # Remove disturbing multiple spaces and tabs
  wip_1="$(echo1 "$1" | sed 's/[ 	][ 	]*/ /g' | \
           sed 's/\(\\\)* / /g' | sed 's/^ //' | sed 's/ $//')";
  wip_noarg="$(echo1 "${wip_1}" | sed 's/ -.*$//')";
  exit_test;

  if obj wip_noarg is_empty
  then
    eval ${_UNSET} wip_1;
    eval ${_UNSET} wip_noarg;
    eval "${return_bad}";
  fi;

  case "${wip_1}" in
  *\ -*)
    wip_args="$(echo1 "${wip_1}" |
                eval sed "'s#^${wip_noarg} ##'")";
    exit_test;
    ;;
  *)
    wip_args='';
    ;;
  esac;

  wip_result='';
### where_is_prog()

  if test -f "${wip_noarg}" && test -x "${wip_noarg}"
  then
    list_append wip_result "${wip_noarg}" "${wip_args}";
    exit_test;
    obj wip_result echo1;
    exit_test;
    eval ${_UNSET} wip_1;
    eval ${_UNSET} wip_args;
    eval ${_UNSET} wip_noarg;
    eval ${_UNSET} wip_result;
    eval "${return_ok}";
  fi;

  # test whether $wip_noarg has directory, so it is not tested with $PATH
  case "${wip_noarg}" in
  */*)
    # now $wip_noarg (with /) is not an executable file

    # test name with space
    obj_from_output wip_name base_name "${wip_noarg}";
    obj_from_output wip_dir dir_name "${wip_noarg}";
    case "${wip_name}" in
    *\ *)
      wip_base="$(echo1 "${wip_name}" | sed 's/ .*$//')";
      exit_test;
      obj_from_output wip_file dir_name_append "${wip_dir}" "${wip_base}";
      exit_test;
### where_is_prog()
      if test -f "${wip_file}" && test -x "${wip_file}"
      then
        wip_baseargs="$(echo1 "${wip_name}" |
                        eval sed "'s#^${wip_base} ##'")";
        exit_test;
        if obj wip_args is_empty
        then
          wip_args="${wip_baseargs}";
        else
          wip_args="${wip_baseargs} ${wip_args}";
        fi;

        list_append wip_result "${wip_file}" "${wip_args}";
        exit_test;
        obj wip_result echo1;
        exit_test;
        eval ${_UNSET} wip_1;
        eval ${_UNSET} wip_args;
        eval ${_UNSET} wip_base;
        eval ${_UNSET} wip_baseargs;
        eval ${_UNSET} wip_dir;
        eval ${_UNSET} wip_file;
        eval ${_UNSET} wip_name;
        eval ${_UNSET} wip_noarg;
        eval ${_UNSET} wip_result;
        eval "${return_ok}";
      fi; # test ${wip_file}
      ;;
    esac; # end of test name with space

### where_is_prog()
    eval ${_UNSET} wip_1;
    eval ${_UNSET} wip_args;
    eval ${_UNSET} wip_base;
    eval ${_UNSET} wip_dir;
    eval ${_UNSET} wip_name;
    eval ${_UNSET} wip_noarg;
    eval ${_UNSET} wip_result;
    eval "${return_bad}";
    ;;
  esac; # test of $wip_noarg on path with directory


  # now $wip_noarg does not have a /, so it is checked with $PATH.

  eval set x "$(path_list "${PATH}")";
  exit_test;
  shift;

  # test path with $win_noarg, evt. with spaces
  for d
  do
    wip_dir="$d";
    obj_from_output wip_file dir_name_append "${wip_dir}" "${wip_noarg}";
### where_is_prog()

    # test $win_file on executable file
    if test -f "${wip_file}" && test -x "${wip_file}"
    then
      list_append wip_result "${wip_file}" "${wip_args}";
      exit_test;
      obj wip_result echo1;
      exit_test;
      eval ${_UNSET} wip_1;
      eval ${_UNSET} wip_dir;
      eval ${_UNSET} wip_file;
      eval ${_UNSET} wip_noarg;
      eval ${_UNSET} wip_result;
      eval "${return_ok}";
    fi; # test $win_file on executable file
  done; # test path with $win_prog with spaces

  case "${wip_noarg}" in
  *\ *)
    # test on path with base name without space
    wip_base="$(echo1 "${wip_noarg}" | sed 's/^\([^ ]*\) .*$/\1/')";
    exit_test;
    for d
    do
      wip_dir="$d";
      obj_from_output wip_file dir_name_append "${wip_dir}" "${wip_base}";
      exit_test;
### where_is_prog()

      # test $win_file on executable file
      if test -f "${wip_file}" && test -x "${wip_file}"
      then
        wip_baseargs="$(echo1 "${wip_noarg}" |
                        sed 's/[^ ]* \(.*\)$/\1/')";
        exit_test;
        if obj wip_args is_empty
        then
          wip_args="${wip_baseargs}";
        else
          wip_args="${wip_args} ${wip_baseargs}";
        fi;
        list_append wip_result "${wip_file}" "${wip_args}";
        exit_test;
        obj wip_result echo1;
        exit_test;
        eval ${_UNSET} wip_1;
        eval ${_UNSET} wip_args;
        eval ${_UNSET} wip_base;
        eval ${_UNSET} wip_baseargs;
        eval ${_UNSET} wip_dir;
        eval ${_UNSET} wip_file;
        eval ${_UNSET} wip_name;
        eval ${_UNSET} wip_noarg;
        eval ${_UNSET} wip_result;
        eval "${return_ok}";
      fi; # test of $wip_file on executable file
    done; # test path with base name without space
### where_is_prog()
    ;;
  esac; # test of $wip_noarg on space

  eval ${_UNSET} wip_1;
  eval ${_UNSET} wip_args;
  eval ${_UNSET} wip_base;
  eval ${_UNSET} wip_baseargs;
  eval ${_UNSET} wip_dir;
  eval ${_UNSET} wip_file;
  eval ${_UNSET} wip_name;
  eval ${_UNSET} wip_noarg;
  eval ${_UNSET} wip_result;
  eval "${return_bad}";
} # where_is_prog()


########################################################################
#                        main* Functions
########################################################################

# The main area contains the following parts:
# - main_init(): initialize temporary files and set exit trap
# - main_parse_MANOPT(): parse $MANOPT
# - main_parse_args(): argument parsing
# - main_set_mode (): determine the display mode
# - main_do_fileargs(): process filespec arguments
# - main_set_resources(): setup X resources
# - main_display(): do the displaying
# - main(): the main function that calls all main_*()


#######################################################################
# main_init ()
#
# Set exit trap and create temporary directory and some temporary files.
#
# Globals: $_TMP_DIR, $_TMP_CAT, $_TMP_STDIN
#
# Variable prefix: mi
#
main_init()
{
  func_check main_init = 0 "$@";
  # call clean_up() on shell termination.
  trap_set;

  # create temporary directory
  umask 0077;
  _TMP_DIR='';
  for d in "${GROFF_TMPDIR}" "${TMPDIR}" "${TMP}" "${TEMP}" \
           "${TEMPDIR}" "${HOME}"'/tmp' '/tmp' "${HOME}" '.'
  do
    mi_dir="$d";
    if obj mi_dir is_empty || obj mi_dir is_not_dir || \
       obj mi_dir is_not_writable
    then
      continue;
    fi;

    case "${mi_dir}" in
    */)
      _TMP_DIR="${mi_dir}";
      ;;
    *)
      _TMP_DIR="${mi_dir}"'/';
      ;;
    esac;
    _TMP_DIR="${_TMP_DIR}groffer${_PROCESS_ID}";
    if obj _TMP_DIR rm_tree
    then
      :
    else
      mi_tdir_="${_TMP_DIR}"_;
      mi_n=1;
      mi_tdir_n="${mi_tdir_}${mi_n}";
### main_init()
      while obj mi_tdir_n is_existing
      do
        if obj mi_tdir_n rm_tree
        then
          # directory could not be removed
          mi_n="$(expr "${mi_n}" + 1)";
          mi_tdir_n="${mi_tdir_}${mi_n}";
          continue;
        fi;
      done;
      _TMP_DIR="${mi_tdir_n}";
    fi;
    eval mkdir "${_TMP_DIR}";
    if is_not_equal "$?" 0
    then
      obj _TMP_DIR rm_tree;
      _TMP_DIR='';
      continue;
    fi;
    if obj _TMP_DIR is_dir && obj _TMP_DIR is_writable
    then
      # $_TMP_DIR can now be used as temporary directory
      break;
    fi;
    obj _TMP_DIR rm_tree;
    _TMP_DIR='';
    continue;
  done;
  if obj _TMP_DIR is_empty
  then
    error "main_init(): \
Couldn't create a directory for storing temporary files.";
  fi;
### main_init()
  if obj _DEBUG_PRINT_TMPDIR is_yes
  then
    echo2 "temporary directory: ${_TMP_DIR}";
  fi;

  obj_from_output _TMP_CAT tmp_create groffer_cat;
  obj_from_output _TMP_STDIN tmp_create groffer_input;

  eval ${_UNSET} mi_dir;
  eval ${_UNSET} mi_n;
  eval ${_UNSET} mi_tdir_;
  eval ${_UNSET} mi_tdir_n;
  eval "${return_ok}";
} # main_init()


########################################################################
# main_parse_MANOPT ()
#
# Parse $MANOPT to retrieve man options, but only if it is a non-empty
# string; found man arguments can be overwritten by the command line.
#
# Globals:
#   in: $MANOPT, $_OPTS_MANOPT_*
#   out: $_MANOPT_*
#
# Variable prefix: mpm
#
main_parse_MANOPT()
{
  func_check main_parse_MANOPT = 0 "$@";

  if obj MANOPT is_not_empty
  then
    # Delete leading and final spaces
    MANOPT="$(echo1 "${MANOPT}" | sed '
s/^'"${_SPACE_SED}"'*//
s/'"${_SPACE_SED}"'*$//
')";
    exit_test;
  fi;
  if obj MANOPT is_empty
  then
    eval "${return_ok}";
  fi;

  mpm_list='';
  # add arguments in $MANOPT by mapping them to groffer options
  eval set x "$(list_from_cmdline _OPTS_MANOPT "${MANOPT}")";
  exit_test;
  shift;
  until test "$#" -le 0 || is_equal "$1" '--'
  do
    mpm_opt="$1";
    shift;
    case "${mpm_opt}" in
    -7|--ascii)
      list_append mpm_list '--ascii';
      ;;
    -a|--all)
      list_append mpm_list '--all';
      ;;
### main_parse_MANOPT()
    -c|--catman)
      do_nothing;
      shift;
      ;;
    -d|--debug)
      do_nothing;
      ;;
    -D|--default)
      # undo all man options so far
      mpm_list='';
      ;;
    -e|--extension)
      list_append mpm_list '--extension';
      shift;
      ;;
    -f|--whatis)
      list_append mpm_list '--whatis';
      shift;
      ;;
    -h|--help)
      do_nothing;
      ;;
    -k|--apropos)
      # groffer's --apropos takes an argument, but man's does not, so
      do_nothing;
      ;;
    -l|--local-file)
      do_nothing;
      ;;
    -L|--locale)
      list_append mpm_list '--locale' "$1";
      shift;
      ;;
### main_parse_MANOPT()
    -m|--systems)
      list_append mpm_list '--systems' "$1";
      shift;
      ;;
    -M|--manpath)
      list_append mpm_list '--manpath' "$1";
      shift;
      ;;
    -p|--preprocessor)
      do_nothing;
      shift;
      ;;
    -P|--pager)
      list_append mpm_list '--pager' "$1";
      shift;
      ;;
    -r|--prompt)
      do_nothing;
      shift;
      ;;
    -S|--sections)
      list_append mpm_list '--sections' "$1";
      shift;
      ;;
    -t|--troff)
      do_nothing;
      ;;
    -T|--device)
      list_append mpm_list '-T' "$1";
      shift;
      ;;
### main_parse_MANOPT()
    -u|--update)
      do_nothing;
      ;;
    -V|--version)
      do_nothing;
      ;;
    -w|--where|--location)
      list_append mpm_list '--location';
      ;;
    -Z|--ditroff)
      do_nothing;
      ;;
    # ignore all other options
    esac;
  done;

  # prepend $mpm_list to the command line
  if obj mpm_list is_not_empty
  then
    eval set x "${mpm_list}" '"$@"';
    shift;
  fi;

  eval ${_UNSET} mpm_list;
  eval ${_UNSET} mpm_opt;
  eval "${return_ok}";
} # main_parse_MANOPT()


########################################################################
# main_parse_args (<command_line_args>*)
#
# Parse arguments; process options and filespec parameters.
#
# Arguments: pass the command line arguments unaltered.
# Globals:
#   in:  $_OPTS_*
#   out: $_OPT_*, $_ADDOPTS, $_FILEARGS
#
# Variable prefix: mpa
#
main_parse_args()
{
  func_check main_parse_args '>=' 0 "$@";
  obj_from_output _ALL_PARAMS list_from_cmdline_with_minus _OPTS_CMDLINE "$@";
  if obj _DEBUG_PRINT_PARAMS is_yes
  then
    echo2 "parameters: ${_ALL_PARAMS}";
  fi;
  eval set x "${_ALL_PARAMS}";
  shift;

  # By the call of `eval', unnecessary quoting was removed.  So the
  # positional shell parameters ($1, $2, ...) are now guaranteed to
  # represent an option or an argument to the previous option, if any;
  # then a `--' argument for separating options and
  # parameters; followed by the filespec parameters if any.

  # Note, the existence of arguments to options has already been checked.
  # So a check for `$#' or `--' should not be done for arguments.

  until test "$#" -le 0 || is_equal "$1" '--'
  do
    mpa_opt="$1";		# $mpa_opt is fed into the option handler
    shift;
    case "${mpa_opt}" in
    -h|--help)
      usage;
      leave;
      ;;
    -Q|--source)		# output source code (`Quellcode').
      _OPT_MODE='source';
      ;;
### main_parse_args()
    -T|--device|--troff-device) # device; arg
      _OPT_DEVICE="$1";
      _check_device_with_mode;
      shift;
      ;;
    -v|--version)
      version;
      leave;
      ;;
    -V)
      _OPT_V='yes';
      ;;
    -Z|--ditroff|--intermediate-output) # groff intermediate output
      _OPT_Z='yes';
      ;;
    -X)
      _OPT_MODE=X;
      ;;
    -?)
      # delete leading `-'
      mpa_optchar="$(echo1 "${mpa_opt}" | sed 's/^-//')";
      exit_test;
      if list_has _OPTS_GROFF_SHORT_NA "${mpa_optchar}"
      then
        list_append _ADDOPTS_GROFF "${mpa_opt}";
      elif list_has _OPTS_GROFF_SHORT_ARG "${mpa_optchar}"
      then
        list_append _ADDOPTS_GROFF "${mpa_opt}" "$1";
        shift;
### main_parse_args()
      else
        error "main_parse_args(): Unknown option : \`$1'";
      fi;
      ;;
    --all)
        _OPT_ALL='yes';
        ;;
    --apropos)			# run `apropos'
      _OPT_APROPOS='yes';
      _APROPOS_SECTIONS='';
      _OPT_WHATIS='no';
      ;;
    --apropos-data)		# run `apropos' for data sections
      _OPT_APROPOS='yes';
      _APROPOS_SECTIONS='457';
      _OPT_WHATIS='no';
      ;;
    --apropos-devel)		# run `apropos' for development sections
      _OPT_APROPOS='yes';
      _APROPOS_SECTIONS='239';
      _OPT_WHATIS='no';
      ;;
    --apropos-progs)		# run `apropos' for program sections
      _OPT_APROPOS='yes';
      _APROPOS_SECTIONS='168';
      _OPT_WHATIS='no';
      ;;
### main_parse_args()
    --ascii)
      list_append _ADDOPTS_GROFF '-mtty-char';
      if obj _OPT_MODE is_empty
      then
        _OPT_MODE='text';
      fi;
      ;;
    --auto)			# the default automatic mode
      _OPT_MODE='';
      ;;
    --bd|--bordercolor)		# border color for viewers, arg;
      _OPT_BD="$1";
      shift;
      ;;
    --bg|--backgroud)		# background color for viewers, arg;
      _OPT_BG="$1";
      shift;
      ;;
    --bw|--borderwidth)		# border width for viewers, arg;
      _OPT_BW="$1";
      shift;
      ;;
    --debug|--debug-all|--debug-filenames|--debug-func|--debug-not-func|\
--debug-grog|--debug-keep|--debug-lm|--debug-params|--debug-shell|\
--debug-stacks|--debug-tmpdir|--debug-user)
      # debug is handled at the beginning
      :;
      ;;
    --default)			# reset variables to default
      reset;
      ;;
### main_parse_args()
    --default-modes)		# sequence of modes in auto mode; arg
      _OPT_DEFAULT_MODES="$1";
      shift;
      ;;
    --display)			# set X display, arg
      _OPT_DISPLAY="$1";
      shift;
      ;;
    --do-nothing)
      _OPT_DO_NOTHING='yes';
      ;;
    --dvi)
      _OPT_MODE='dvi';
      ;;
    --dvi-viewer|--dvi-viewer-tty) # viewer program for dvi mode; arg
      _OPT_VIEWER_DVI="$1";
      shift;
      ;;
    --extension)		# the extension for man pages, arg
      _OPT_EXTENSION="$1";
      shift;
      ;;
### main_parse_args()
    --fg|--foreground)		# foreground color for viewers, arg;
      _OPT_FG="$1";
      shift;
      ;;
    --fn|--ft|--font)		# set font for viewers, arg;
      _OPT_FN="$1";
      shift;
      ;;
    --geometry)			# window geometry for viewers, arg;
      _OPT_GEOMETRY="$1";
      shift;
      ;;
    --groff)
      _OPT_MODE='groff';
      ;;
    --html|--www)		# display with web browser
      _OPT_MODE=html;
      ;;
    --html-viewer|--www-viewer|--html-viewer-tty|--www-viewer-tty)
      # viewer program for html mode; arg
      _OPT_VIEWER_HTML="$1";
      shift;
      ;;
    --iconic)			# start viewers as icons
      _OPT_ICONIC='yes';
      ;;
### main_parse_args()
    --locale)			# set language for man pages, arg
      # argument is xx[_territory[.codeset[@modifier]]] (ISO 639,...)
      _OPT_LANG="$1";
      shift;
      ;;
    --local-file)		# force local files; same as `--no-man'
      _MAN_FORCE='no';
      _MAN_ENABLE='no';
      ;;
    --location|--where)		# print file locations to stderr
      _DEBUG_PRINT_FILENAMES='yes';
      ;;
    --man)		       # force all file params to be man pages
      _MAN_ENABLE='yes';
      _MAN_FORCE='yes';
      ;;
    --manpath)		      # specify search path for man pages, arg
      # arg is colon-separated list of directories
      _OPT_MANPATH="$1";
      shift;
      ;;
    --mode)			# display mode
      mpa_arg="$1";
      shift;
      case "${mpa_arg}" in
      auto|'')		     # search mode automatically among default
        _OPT_MODE='';
        ;;
      groff)			# pass input to plain groff
        _OPT_MODE='groff';
        ;;
### main_parse_args()
      html|www)			# display with a web browser
        _OPT_MODE='html';
        ;;
      dvi)			# display with xdvi viewer
        _OPT_MODE='dvi';
        ;;
      pdf)			# display with PDF viewer
        _OPT_MODE='pdf';
        ;;
      ps)			# display with Postscript viewer
        _OPT_MODE='ps';
        ;;
      text)			# output on terminal
        _OPT_MODE='text';
        ;;
      tty)			# output on terminal
        _OPT_MODE='tty';
        ;;
      X|x)			# output on X roff viewer
        _OPT_MODE='x';
        ;;
### main_parse_args()
      Q|source)			# display source code
        _OPT_MODE="source";
        ;;
      *)
        error "main_parse_args(): unknown mode ${mpa_arg}";
        ;;
      esac;
      ;;
    --no-location)		# disable former call to `--location'
      _DEBUG_PRINT_FILENAMES='no';
      ;;
    --no-man)			# disable search for man pages
      # the same as --local-file
      _MAN_FORCE='no';
      _MAN_ENABLE='no';
      ;;
    --no-special)		# disable some special former calls
      _OPT_ALL='no'
      _OPT_APROPOS='no'
      _OPT_WHATIS='no'
      ;;
    --pager|--tty-viewer|--tty-viewer-tty)
      # set paging program for tty mode, arg
      _OPT_PAGER="$1";
      shift;
      ;;
    --pdf)
      _OPT_MODE='pdf';
      ;;
### main_parse_args()
    --pdf-viewer|--pdf-viewer-tty) # viewer program for pdf mode; arg
      _OPT_VIEWER_PDF="$1";
      shift;
      ;;
    --print)			# for argument test
      echo2 "$1";
      shift;
      ;;
    --ps)
      _OPT_MODE='ps';
      ;;
    --ps-viewer|--ps-viewer-tty) # viewer program for ps mode; arg
      _OPT_VIEWER_PS="$1";
      shift;
      ;;
### main_parse_args()
    --resolution)		# set resolution for X devices, arg
      mpa_arg="$1";
      shift;
      case "${mpa_arg}" in
      75|75dpi)
        mpa_dpi=75;
        ;;
      100|100dpi)
        mpa_dpi=100;
        ;;
      *)
        error "main_parse_args(): \
only resoutions of 75 or 100 dpi are supported";
        ;;
      esac;
      _OPT_RESOLUTION="${mpa_dpi}";
      ;;
    --rv)
      _OPT_RV='yes';
      ;;
    --sections)			# specify sections for man pages, arg
      # arg is colon-separated list of section names
      _OPT_SECTIONS="$1";
      shift;
      ;;
    --shell)
      # already done during the first run; so ignore the argument
      shift;
      ;;
### main_parse_args()
    --systems)			# man pages for different OS's, arg
      # argument is a comma-separated list
      _OPT_SYSTEMS="$1";
      shift;
      ;;
    --text)			# text mode without pager
      _OPT_MODE=text;
      ;;
    --title)			# title for X viewers; arg
      if is_not_empty "$1"
      then
        list_append _OPT_TITLE "$1";
      fi;
      shift;
      ;;
     --to-stdout)		# print mode file without display
      _OPT_STDOUT='yes';
      ;;
     --tty)			# tty mode, text with pager
      _OPT_MODE=tty;
      ;;
    --text-device|--tty-device) # device for tty mode; arg
      _OPT_TEXT_DEVICE="$1";
      shift;
      ;;
    --whatis)
      _OPT_WHATIS='yes';
      _OPT_APROPOS='no';
      ;;
    --X|--x)
      _OPT_MODE=x;
      ;;
### main_parse_args()
    --xrm)			# pass X resource string, arg;
      list_append _OPT_XRM "$1";
      shift;
      ;;
    --x-viewer|--X-viewer|--x-viewer-tty|--X-viewer-tty)
      # viewer program for x mode; arg
      _OPT_VIEWER_X="$1";
      shift;
      ;;
    *)
      error 'main_parse_args(): unknown option '"\`${mpa_opt}'.";
      ;;
    esac;
  done;
  shift;			# remove `--' argument

  if obj _OPT_WHATIS is_yes
  then
    _MAN_ALL='yes';
    _APROPOS_SECTIONS='';
  fi;

  if obj _OPT_DO_NOTHING is_yes
  then
    leave;
  fi;

### main_parse_args()
  case "$_OPT_DEFAULT_MODES" in
  '') :; ;;
  *,*)
    obj_from_output _OPT_DEFAULT_MODES \
      obj _OPT_DEFAULT_MODES list_from_split ',';
    ;;
  *) :; ;;
  esac;

  # Remaining arguments are file names (filespecs).
  # Save them to list $_FILEARGS
  if is_equal "$#" 0
  then				# use "-" for standard input
    _NO_FILESPECS='yes';
    set x '-';
    shift;
  fi;
  _FILEARGS='';
  list_append _FILEARGS "$@";
  # $_FILEARGS must be retrieved with `eval set x "$_FILEARGS"; shift;'
  eval ${_UNSET} mpa_arg;
  eval ${_UNSET} mpa_dpi;
  eval ${_UNSET} mpa_opt;
  eval ${_UNSET} mpa_optchar;
  eval "${return_ok}";
} # main_parse_args()


# Called from main_parse_args() because double `case' is not possible.
# Globals: $_OPT_DEVICE, $_OPT_MODE
_check_device_with_mode()
{
  func_check _check_device_with_mode = 0 "$@";
  case "${_OPT_DEVICE}" in
  dvi)
    _OPT_MODE=dvi;
    eval "${return_ok}";
    ;;
  html)
    _OPT_MODE=html;
    eval "${return_ok}";
    ;;
  lbp|lj4)
    _OPT_MODE=groff;
    eval "${return_ok}";
    ;;
  ps)
    _OPT_MODE=ps;
    eval "${return_ok}";
    ;;
  ascii|cp1047|latin1|utf8)
    if obj _OPT_MODE is_not_equal text
    then
      _OPT_MODE=tty;		# default text mode
    fi;
    eval "${return_ok}";
    ;;
  X*)
    _OPT_MODE=x;
    eval "${return_ok}";
    ;;
  *)				# unknown device, go to groff mode
    _OPT_MODE=groff;
    eval "${return_ok}";
    ;;
  esac;
  eval "${return_error}";
} # _check_device_with_mode() of main_parse_args()


########################################################################
# main_set_mode ()
#
# Determine the display mode and the corresponding viewer program.
#
# Globals:
#   in:  $DISPLAY, $_OPT_MODE, $_OPT_DEVICE
#   out: $_DISPLAY_MODE
#
# Variable prefix: msm
#
main_set_mode()
{
  func_check main_set_mode = 0 "$@";

  # set display
  if obj _OPT_DISPLAY is_not_empty
  then
    DISPLAY="${_OPT_DISPLAY}";
  fi;

  if obj _OPT_V is_yes
  then
    list_append _ADDOPTS_GROFF '-V';
  fi;
  if obj _OPT_Z is_yes
  then
    _DISPLAY_MODE='groff';
    list_append _ADDOPTS_GROFF '-Z';
  fi;
  if obj _OPT_MODE is_equal 'groff'
  then
    _DISPLAY_MODE='groff';
  fi;
  if obj _DISPLAY_MODE is_equal 'groff'
  then
    eval ${_UNSET} msm_modes;
    eval ${_UNSET} msm_viewers;
    eval "${return_ok}";
  fi;

### main_set_mode()

  case "${_OPT_MODE}" in
  '')				# automatic mode
    case "${_OPT_DEVICE}" in
    X*)
     if is_not_X
      then
        error_user "no X display found for device ${_OPT_DEVICE}";
      fi;
      _DISPLAY_MODE='x';
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    ascii|cp1047|latin1|utf8)
      if obj _DISPLAY_MODE is_not_equal 'text'
      then
        _DISPLAY_MODE='tty';
      fi;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
### main_set_mode()
    esac;
    if is_not_X
    then
      _DISPLAY_MODE='tty';
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
    fi;

    if obj _OPT_DEFAULT_MODES is_empty
    then
      msm_modes="${_DEFAULT_MODES}";
    else
      msm_modes="${_OPT_DEFAULT_MODES}";
    fi;
    ;;
  source)
    _DISPLAY_MODE='source';
    eval ${_UNSET} msm_modes;
    eval ${_UNSET} msm_viewers;
    eval "${return_ok}";
    ;;
  text)
    _DISPLAY_MODE='text';
    eval ${_UNSET} msm_modes;
    eval ${_UNSET} msm_viewers;
    eval "${return_ok}";
    ;;
  tty)
    _DISPLAY_MODE='tty';
    eval ${_UNSET} msm_modes;
    eval ${_UNSET} msm_viewers;
    eval "${return_ok}";
    ;;
### main_set_mode()
  html)
    _DISPLAY_MODE='html';
    msm_modes="${_OPT_MODE}";
    ;;
  *)				# display mode was given
    msm_modes="${_OPT_MODE}";
    ;;
  esac;

  eval set x "${msm_modes}";
  shift;
  while is_greater_than "$#" 0
  do
    msm_1="$1";
    shift;

    _VIEWER_BACKGROUND='no';

    case "${msm_1}" in
    dvi)
      _get_prog_args DVI;
      if is_not_equal "$?" 0
      then
        continue;
      fi;
      if obj _DISPLAY_PROG is_empty
      then
        if is_equal "$#" 0
        then
          error 'main_set_mode(): No viewer for dvi mode available.';
        else
          continue;
        fi;
      fi;
### main_set_mode()
      _DISPLAY_MODE="dvi";
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    html)
      _get_prog_args HTML;
      if is_not_equal "$?" 0
      then
        continue;
      fi;
      if obj _DISPLAY_PROG is_empty
      then
        if is_equal "$#" 0
        then
          error 'main_set_mode(): No viewer for html mode available.';
        else
          continue;
        fi;
      fi;
### main_set_mode()
      _DISPLAY_MODE=html;
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    pdf)
      if obj _PDF_DID_NOT_WORK is_yes
      then
        if is_equal "$#" 0
        then
          error 'main_set_mode(): pdf mode did not work.';
        else
          continue;
        fi;
      fi;
      if obj _PDF_HAS_PS2PDF is_not_yes
      then
        if is_prog ps2pdf
        then
          _PDF_HAS_PS2PDF='yes';
        fi;
      fi;
      if obj _PDF_HAS_GS is_not_yes
      then
        if is_prog gs
        then
          _PDF_HAS_GS='yes';
        fi;
      fi;
      _get_prog_args PDF;
      if is_not_equal "$?" 0
      then
        _PDF_DID_NOT_WORK='yes';
        continue;
      fi;
      if obj _DISPLAY_PROG is_empty
      then
        _PDF_DID_NOT_WORK='yes';
        if is_equal "$#" 0
        then
          error 'main_set_mode(): No viewer for pdf mode available.';
        else
          continue;
        fi;
      fi;
      _DISPLAY_MODE="pdf";
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
### main_set_mode()
    ps)
      _get_prog_args PS;
      if is_not_equal "$?" 0
      then
        continue;
      fi;
      if obj _DISPLAY_PROG is_empty
      then
        if is_equal "$#" 0
        then
          error 'main_set_mode(): No viewer for ps mode available.';
        else
          continue;
        fi;
      fi;
      _DISPLAY_MODE="ps";
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    text)
      _DISPLAY_MODE='text';
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
### main_set_mode()
    tty)
      _DISPLAY_MODE='tty';
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    x)
      _get_prog_args x;
      if is_not_equal "$?" 0
      then
        continue;
      fi;
      if obj _DISPLAY_PROG is_empty
      then
        if is_equal "$#" 0
        then
          error 'main_set_mode(): No viewer for x mode available.';
        else
          continue;
        fi;
      fi;
      _DISPLAY_MODE='x';
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
### main_set_mode()
    X)
      _DISPLAY_MODE='X';
      eval ${_UNSET} msm_1;
      eval ${_UNSET} msm_modes;
      eval ${_UNSET} msm_viewers;
      eval "${return_ok}";
      ;;
    esac;
  done;
  eval ${_UNSET} msm_1;
  eval ${_UNSET} msm_modes;
  eval ${_UNSET} msm_viewers;
  error_user "No suitable display mode found.";
} # main_set_mode()


# _get_prog_args (<MODE>)
#
# Simplification for loop in main_set_mode().
#
# Globals in/out: $_VIEWER_BACKGROUND
# Globals in    : $_OPT_VIEWER_<MODE>, $_VIEWER_<MODE>_X, $_VIEWER_<MODE>_TTY
#
# Variable prefix: _gpa
#
_get_prog_args()
{
  func_check _get_prog_args '=' 1 "$@";

  x="$(echo1 $1 | tr [a-z] [A-Z])";
  eval _gpa_opt='"${_OPT_VIEWER_'"$x"'}"';
  _gpa_xlist=_VIEWER_"$x"_X;
  _gpa_ttylist=_VIEWER_"$x"_TTY;

  if obj _gpa_opt is_empty
  then
    _VIEWER_BACKGROUND='no';
    if is_X
    then
      _get_first_prog "${_gpa_xlist}";
      x="$?";
      if is_equal "$x" 0
      then
        _VIEWER_BACKGROUND='yes';
      fi;
    else
      _get_first_prog "${_gpa_ttylist}";
      x="$?";
    fi;
    exit_test;
    eval ${_UNSET} _gpa_opt;
    eval ${_UNSET} _gpa_prog;
    eval ${_UNSET} _gpa_ttylist;
    eval ${_UNSET} _gpa_xlist;
    eval "${return_var} $x";
### _get_prog_args() of main_set_mode()
  else				# $_gpa_opt is not empty
    obj_from_output _gpa_prog where_is_prog "${_gpa_opt}";
    if is_not_equal "$?" 0 || obj _gpa_prog is_empty
    then
      exit_test;
      echo2 "_get_prog_args(): '${_gpa_opt}' is not an existing program.";
      eval ${_UNSET} _gpa_opt;
      eval ${_UNSET} _gpa_prog;
      eval ${_UNSET} _gpa_ttylist;
      eval ${_UNSET} _gpa_xlist;
      eval "${return_bad}";
    fi;
    exit_test;

    # $_gpa_prog from opt is an existing program

### _get_prog_args() of main_set_mode()
    if is_X
    then
      eval _check_prog_on_list ${_gpa_prog} ${_gpa_xlist};
      if is_equal "$?" 0
      then
        _VIEWER_BACKGROUND='yes';
      else
        _VIEWER_BACKGROUND='no';
        eval _check_prog_on_list ${_gpa_prog} ${_gpa_ttylist};
      fi;
    else			# is not X
      _VIEWER_BACKGROUND='no';
      eval _check_prog_on_list ${_gpa_prog} ${_gpa_ttylist};
    fi;				# is_X
  fi;				# test of $_gpa_opt
  eval ${_UNSET} _gpa_opt;
  eval ${_UNSET} _gpa_prog;
  eval ${_UNSET} _gpa_ttylist;
  eval ${_UNSET} _gpa_xlist;
  eval "${return_good}";
} # _get_prog_args() of main_set_mode()


# _get_first_prog (<prog_list_name>)
#
# Retrieve from the elements of the list in the argument the first
# existing program in $PATH.
#
# Local function for main_set_mode().
#
# Return  : `1' if none found, `0' if found.
# Output  : none
#
# Variable prefix: _gfp
#
_get_first_prog()
{
  func_check _get_first_prog '=' 1 "$@";
  eval x='"${'"$1"'}"';
  eval set x "$x";
  shift;
  for i
  do
    _gfp_i="$i";
    if obj _gfp_i is_empty
    then
      continue;
    fi;
    obj_from_output _gfp_result where_is_prog "${_gfp_i}";
    if is_equal "$?" 0 && obj _gfp_result is_not_empty
    then
      exit_test;
      eval set x ${_gfp_result};
      shift;
      _DISPLAY_PROG="$1";
      _DISPLAY_ARGS="$2";
      eval ${_UNSET} _gfp_i;
      eval ${_UNSET} _gfp_result;
      eval "${return_good}";
    fi;
    exit_test;
  done;
  eval ${_UNSET} _gfp_i;
  eval ${_UNSET} _gfp_result;
  eval "${return_bad}";
} # _get_first_prog() of main_set_mode()


# _check_prog_on_list (<prog> <args> <prog_list_name>)
#
# Check whether the content of <prog> is in the list <prog_list_name>.
# The globals are set correspondingly.
#
# Local function for main_set_mode().
#
# Arguments: 3
#
# Return  : `1' if not a part of the list, `0' if found in the list.
# Output  : none
#
# Globals in    : $_VIEWER_<MODE>_X, $_VIEWER_<MODE>_TTY
# Globals in/out: $_DISPLAY_PROG, $_DISPLAY_ARGS
#
# Variable prefix: _cpol
#
_check_prog_on_list()
{
  func_check _check_prog_on_list '=' 3 "$@";
  _DISPLAY_PROG="$1";
  _DISPLAY_ARGS="$2";

  eval _cpol_3='"${'"$3"'}"';
  eval set x "${_cpol_3}";
  shift;
  eval ${_UNSET} _cpol_3;

  for i
  do
    _cpol_i="$i";
    obj_from_output _cpol_list where_is_prog "${_cpol_i}";
    if is_not_equal "$?" 0 || obj _cpol_list is_empty
    then
      exit_test;
      continue;
    fi;
    exit_test;
    _cpol_prog="$(eval set x ${_cpol_list}; shift; echo1 "$1")";

    if is_not_equal "${_DISPLAY_PROG}" "${_cpol_prog}"
    then
      exit_test;
      continue;
    fi;
    exit_test;
### _check_prog_on_list() of main_set_mode()

    # equal, prog found

    _cpol_args="$(eval set x ${_cpol_list}; shift; echo1 "$2")";
    eval ${_UNSET} _cpol_list;
    if obj _cpol_args is_not_empty
    then
      if obj _DISPLAY_ARGS is_empty
      then
        _DISPLAY_ARGS="${_cpol_args}";
      else
        _DISPLAY_ARGS="${_cpol_args} ${_DISPLAY_ARGS}";
      fi;
    fi;

    eval ${_UNSET} _cpol_i;
    eval ${_UNSET} _cpol_args;
    eval ${_UNSET} _cpol_prog;
    eval "${return_good}";
  done; # for vars in list

  # prog was not in the list
  eval ${_UNSET} _cpol_i;
  eval ${_UNSET} _cpol_args;
  eval ${_UNSET} _cpol_list;
  eval ${_UNSET} _cpol_prog;
  eval "${return_bad}";
} # _check_prog_on_list() of main_set_mode()


#######################################################################
# main_do_fileargs ()
#
# Process filespec arguments.
#
# Globals:
#   in: $_FILEARGS (process with `eval set x "$_FILEARGS"; shift;')
#
# Variable prefix: mdfa
#
main_do_fileargs()
{
  func_check main_do_fileargs = 0 "$@";
  special_setup;
  if obj _OPT_APROPOS is_yes
  then
    if obj _NO_FILESPECS is_yes
    then
      apropos_filespec;
      eval "${return_ok}";
    fi;
  else
    if list_has _FILEARGS '-'
    then
      save_stdin;
    fi;
  fi;
  eval set x "${_FILEARGS}";
  shift;
  eval ${_UNSET} _FILEARGS;
### main_do_fileargs()
  while is_greater_than "$#" 0
  do
    mdfa_filespec="$1";
    _FILESPEC_ARG="$1";
    shift;
    _FILESPEC_IS_MAN='no';
    _TMP_MANSPEC='';
    _SPECIAL_FILESPEC='no';

    case "${mdfa_filespec}" in
    '')
      continue;
      ;;
    esac;

    # check for file
    case "${mdfa_filespec}" in
    '-')
      special_filespec;
      if obj _OPT_APROPOS is_yes
      then
        continue;
      fi;
      register_file '-';
      continue;
      ;;
### main_do_fileargs()
    */*)
      special_filespec;
      if obj _OPT_APROPOS is_yes
      then
        continue;
      fi;
      if obj mdfa_filespec is_file
      then
        obj mdfa_filespec register_file;
      else
        echo2 "The argument ${mdfa_filespec} is not a file.";
      fi;
      continue;
      ;;
    *)
      if obj _OPT_APROPOS is_yes
      then
        special_filespec;
        continue;
      fi;
      # check whether filespec is an existing file
      if obj _MAN_FORCE is_not_yes
      then
        if obj mdfa_filespec is_file
        then
          special_filespec;
          obj mdfa_filespec register_file;
          continue;
        fi;
      fi;
      ;;
    esac;
### main_do_fileargs()

    # now it must be a man page pattern

    if obj _MACRO_PKG is_not_empty && obj _MACRO_PKG is_not_equal '-man'
    then
      echo2 "${mdfa_filespec} is not a file, man pages are ignored "\
"due to ${_MACRO_PKG}.";
      continue;
    fi;

    # check for man page
    if obj _MAN_ENABLE is_not_yes
    then
      echo2 "The argument ${mdfa_filespec} is not a file.";
      continue;
    fi;
    if obj _MAN_FORCE is_yes
    then
      mdfa_errmsg='is not a man page.';
    else
      mdfa_errmsg='is neither a file nor a man page.';
    fi;
### main_do_fileargs()
    man_setup;
    _FILESPEC_IS_MAN='yes';

    # test filespec with `man:...' or `...(...)' on man page
    mdfa_name='';
    mdfa_section='';
    mdfa_ext='';

    mdfa_names="${mdfa_filespec}";
    case "${mdfa_filespec}" in
    man:*)
        mdfa_names="${mdfa_names} "\
"$(obj mdfa_filespec echo1 | sed 's/^man://')";
        ;;
    esac;

    mdfa_continue='no';
    for i in ${mdfa_names}
    do
      mdfa_i=$i;
      if obj mdfa_i man_is_man
      then
        special_filespec;
        obj mdfa_i man_get;
        mdfa_continue='yes';
        break;
      fi;
      case "${mdfa_i}" in
      *\(${_MAN_AUTO_SEC_CHARS}*\))
        mdfa_section="$(obj mdfa_i echo1 | sed 's/^[^(]*(\(.\).*)$/\1/')";
        mdfa_name="$(obj mdfa_i echo1 | sed 's/^\([^(]*\)(.*)$/\1/')";
        mdfa_ext="$(obj mdfa_i echo1 | sed 's/^[^(]*(.\(.*\))$/\1/')";
        if man_is_man "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}"
        then
          special_filespec;
          man_get "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}";
          mdfa_continue='yes';
          break;
        fi;
        ;;
      *.${_MAN_AUTO_SEC_CHARS}*)
        mdfa_name="$(obj mdfa_i echo1 | \
          sed 's/^\(.*\)\.'"${_MAN_AUTO_SEC_CHARS}"'.*$/\1/')";
        mdfa_section="$(obj mdfa_i echo1 | \
          sed 's/^.*\.\('"${_MAN_AUTO_SEC_CHARS}"'\).*$/\1/')";
        mdfa_ext="$(obj mdfa_i echo1 | \
          sed 's/^.*\.'"${_MAN_AUTO_SEC_CHARS}"'\(.*\)$/\1/')";
        if man_is_man "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}"
        then
          special_filespec;
          man_get "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}";
          mdfa_continue='yes';
          break;
        fi;
      ;;
      esac;
    done;

    if obj mdfa_continue is_yes
    then
      continue;
    fi;

### main_do_fileargs()
    # check on "s name", where "s" is a section with or without an extension
    if is_not_empty "$1"
    then
      mdfa_name="$1";
      case "${mdfa_filespec}" in
      ${_MAN_AUTO_SEC_CHARS})
        mdfa_section="${mdfa_filespec}";
        mdfa_ext='';
        ;;
      ${_MAN_AUTO_SEC_CHARS}*)
        mdfa_section="$(echo1 "${mdfa_filespec}" | \
                        sed 's/^\(.\).*$/\1/')";
        mdfa_ext="$(echo1 "${mdfa_filespec}" | \
                    sed 's/^.\(.*\)$/\1/')";
        ;;
      *)
        echo2 "${mdfa_filespec} ${mdfa_errmsg}";
        continue;
        ;;
      esac;
      shift;
      if man_is_man "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}"
      then
        _FILESPEC_ARG="${mdfa_filespec} ${mdfa_name}";
        special_filespec;
        man_get "${mdfa_name}" "${mdfa_section}" "${mdfa_ext}";
        continue;
      else
        echo2 "No man page for ${mdfa_name} with section ${mdfa_filespec}.";
        continue;
      fi;
    fi;

### main_do_fileargs()
    echo2 "${mdfa_filespec} ${mdfa_errmsg}";
    continue;
  done;

  obj _TMP_STDIN rm_file_with_debug;
  eval ${_UNSET} mdfa_filespec;
  eval ${_UNSET} mdfa_i;
  eval ${_UNSET} mdfa_name;
  eval ${_UNSET} mdfa_names;
  eval "${return_ok}";
} # main_do_fileargs()


########################################################################
# main_set_resources ()
#
# Determine options for setting X resources with $_DISPLAY_PROG.
#
# Globals: $_DISPLAY_PROG, $_OUTPUT_FILE_NAME
#
# Variable prefix: msr
#
main_set_resources()
{
  func_check main_set_resources = 0 "$@";
  # $msr_prog   viewer program
  # $msr_rl     resource list
  for f in ${_TMP_DIR}/,man*
  do
    rm_file_with_debug $f;
  done;
  obj_from_output msr_title \
    get_first_essential "${_OPT_TITLE}" "${_REG_TITLE_LIST}";
  _OUTPUT_FILE_NAME='';
  eval set x "${msr_title}";
  shift;
  until is_equal "$#" 0
  do
    msr_n="$1";
    case "${msr_n}" in
    '')
      continue;
      ;;
    ,*)
      msr_n="$(echo1 "$1" | sed 's/^,,*//')";
      exit_test;
      ;;
    esac;
    if obj msr_n is_empty
    then
      continue;
    fi;
    if obj _OUTPUT_FILE_NAME is_not_empty
    then
      _OUTPUT_FILE_NAME="${_OUTPUT_FILE_NAME}"',';
    fi;
    _OUTPUT_FILE_NAME="${_OUTPUT_FILE_NAME}${msr_n}";
    shift;
  done; # until $# is 0
### main_set_resources()

  case "${_OUTPUT_FILE_NAME}" in
  '')
    _OUTPUT_FILE_NAME='-';
    ;;
  ,*)
    error "main_set_resources(): ${_OUTPUT_FILE_NAME} starts with a comma.";
    ;;
  esac;
  _OUTPUT_FILE_NAME="${_TMP_DIR}/${_OUTPUT_FILE_NAME}";

  if obj _DISPLAY_PROG is_empty
  then				# for example, for groff mode
    _DISPLAY_ARGS='';
    eval ${_UNSET} msr_n;
    eval ${_UNSET} msr_prog;
    eval ${_UNSET} msr_rl;
    eval ${_UNSET} msr_title;
    eval "${return_ok}";
  fi;

  eval set x "${_DISPLAY_PROG}";
  shift;
  obj_from_output msr_prog base_name "$1";
  shift;
  if is_greater_than $# 0
  then
    if obj _DISPLAY_ARGS is_empty
    then
      _DISPLAY_ARGS="$*";
    else
      _DISPLAY_ARGS="$* ${_DISPLAY_ARGS}";
    fi;
  fi;
### main_set_resources()
  msr_rl='';
  if obj _OPT_BD is_not_empty
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-bd' "${_OPT_BD}";
      ;;
    esac;
  fi;
  if obj _OPT_BG is_not_empty
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-bg' "${_OPT_BG}";
      ;;
    kghostview)
      list_append msr_rl '--bg' "${_OPT_BG}";
      ;;
    xpdf)
      list_append msr_rl '-papercolor' "${_OPT_BG}";
      ;;
    esac;
  fi;
  if obj _OPT_BW is_not_empty
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      _list_append msr_rl '-bw' "${_OPT_BW}";
      ;;
    esac;
  fi;
### main_set_resources()
  if obj _OPT_FG is_not_empty
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-fg' "${_OPT_FG}";
      ;;
    kghostview)
      list_append msr_rl '--fg' "${_OPT_FG}";
      ;;
    esac;
  fi;
  if is_not_empty "${_OPT_FN}"
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-fn' "${_OPT_FN}";
      ;;
    kghostview)
      list_append msr_rl '--fn' "${_OPT_FN}";
      ;;
    esac;
  fi;
  if is_not_empty "${_OPT_GEOMETRY}"
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi|xpdf)
      list_append msr_rl '-geometry' "${_OPT_GEOMETRY}";
      ;;
    kghostview)
      list_append msr_rl '--geometry' "${_OPT_GEOMETRY}";
      ;;
    esac;
  fi;
### main_set_resources()
  if is_empty "${_OPT_RESOLUTION}"
  then
    _OPT_RESOLUTION="${_DEFAULT_RESOLUTION}";
    case "${msr_prog}" in
    gxditview|xditview)
      list_append msr_rl '-resolution' "${_DEFAULT_RESOLUTION}";
      ;;
    xpdf)
      case "${_DISPLAY_PROG}" in
      *-z*)
        :;
        ;;
      *)			# if xpdf does not have option -z
        case "${_DEFAULT_RESOLUTION}" in
        75)
          # 72dpi is '100'
          list_append msr_rl '-z' '104';
          ;;
        100)
          list_append msr_rl '-z' '139';
          ;;
        esac;
        ;;
      esac;
      ;;
    esac;
  else
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-resolution' "${_OPT_RESOLUTION}";
      ;;
    xpdf)
      case "${_DISPLAY_PROG}" in
      *-z*)
        :;
        ;;
      *)			# if xpdf does not have option -z
        case "${_OPT_RESOLUTION}" in
        75)
          list_append msr_rl '-z' '104';
          # '100' corresponds to 72dpi
          ;;
### main_set_resources()
        100)
          list_append msr_rl '-z' '139';
          ;;
        esac;
        ;;
      esac;
      ;;
    esac;
  fi;
  if is_yes "${_OPT_ICONIC}"
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-iconic';
      ;;
    esac;
  fi;
  if is_yes "${_OPT_RV}"
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi)
      list_append msr_rl '-rv';
      ;;
    esac;
  fi;
  if is_not_empty "${_OPT_XRM}"
  then
    case "${msr_prog}" in
    ghostview|gv|gxditview|xditview|xdvi|xpdf)
      eval set x "${_OPT_XRM}";
      shift;
      for i
      do
        list_append msr_rl '-xrm' "$i";
      done;
### main_set_resources()
      ;;
    esac;
  fi;
  if is_not_empty "${msr_title}"
  then
    case "${msr_prog}" in
    gxditview|xditview)
      list_append msr_rl '-title' "${msr_title}";
      ;;
    esac;
  fi;
  if obj _DISPLAY_ARGS is_empty
  then
    _DISPLAY_ARGS="${msr_rl}";
  else
    _DISPLAY_ARGS="${msr_l} ${_DISPLAY_ARGS}";
  fi;
  eval ${_UNSET} msr_n;
  eval ${_UNSET} msr_prog;
  eval ${_UNSET} msr_rl;
  eval ${_UNSET} msr_title;
  eval "${return_ok}";
} # main_set_resources


########################################################################
# main_display ()
#
# Do the actual display of the whole thing.
#
# Globals:
#   in: $_DISPLAY_MODE, $_OPT_DEVICE, $_ADDOPTS_GROFF,
#       $_TMP_CAT, $_OPT_PAGER, $_MANOPT_PAGER, $_OUTPUT_FILE_NAME
#
# Variable prefix: md
#
main_display()
{
  func_check main_display = 0 "$@";

  export md_addopts;
  export md_groggy;
  export md_modefile;

  if obj _TMP_CAT is_empty_file
  then
    echo2 'groffer: empty input.';
    clean_up;
    eval "${return_ok}";
  fi;

  md_modefile="${_OUTPUT_FILE_NAME}";

  # go to the temporary directory to be able to access internal data files
  cd "${_TMP_DIR}" >"${_NULL_DEV}" 2>&1;

  case "${_DISPLAY_MODE}" in
  groff)
    if obj _OPT_DEVICE is_not_empty
    then
      _ADDOPTS_GROFF="${_ADDOPTS_GROFF} -T${_OPT_DEVICE}";
    fi;
    md_groggy="$(tmp_cat | eval grog)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_opt_V;

### main_display()
    obj md_modefile rm_file;
    mv "${_TMP_CAT}" "${md_modefile}";
    trap_unset;
    cat "${md_modefile}" | \
    {
      trap_set;
      eval "${md_groggy}" "${_ADDOPTS_GROFF}";
    } &
    ;;
  text|tty)
    case "${_OPT_DEVICE}" in
    '')
      obj_from_output md_device \
        get_first_essential "${_OPT_TEXT_DEVICE}" "${_DEFAULT_TTY_DEVICE}";
      ;;
    ascii|cp1047|latin1|utf8)
      md_device="${_OPT_DEVICE}";
      ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}";
      ;;
    esac;
    md_addopts="${_ADDOPTS_GROFF}";
    md_groggy="$(tmp_cat | grog -T${md_device})";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    if obj _DISPLAY_MODE is_equal 'text'
    then
      _do_opt_V;
      tmp_cat | eval "${md_groggy}" "${md_addopts}";
    else			# $_DISPLAY_MODE is 'tty'
### main_display()
      md_pager='';
      for p in "${_OPT_PAGER}" "${_MANOPT_PAGER}" "${PAGER}"
      do
        if obj p is_empty
        then
          continue;
        fi;
        obj_from_output md_pager where_is_prog "$p";
        if is_not_equal "$?" 0 || obj md_pager is_empty
        then
          md_pager='';
          continue;
        fi;
        eval set x $md_pager;
        shift;
        case "$1" in
        */less)
          if is_empty "$2"
          then
            md_pager="$1"' -r -R';
          else
            md_pager="$1"' -r -R '"$2";
          fi;
          ;;
### main_display()
        *)
          if is_empty "$2"
          then
            md_pager="$1";
          else
            md_pager="$1 $2";
          fi;
          ;;
        esac;
        break;
      done;
      if obj md_pager is_empty
      then
        eval set x ${_VIEWER_TTY_TTY} ${_VIEWER_TTY_X} 'cat';
        shift;
        # that is: 'less -r -R' 'more' 'pager' 'xless' 'cat'
        for p
        do
          if obj p is_empty
          then
            continue;
          fi;
          md_p="$p";
          if is_prog "${md_p}"
          then
            md_pager="${md_p}";
            break;
          fi;
        done;
      fi;
### main_display()
      if obj md_pager is_empty
      then
        error 'main_display(): no pager program found for tty mode';
      fi;
      _do_opt_V;
      tmp_cat | eval "${md_groggy}" "${md_addopts}" | \
                eval "${md_pager}";
    fi;				# $_DISPLAY_MODE
    clean_up;
    ;;				# text|tty)
  source)
    tmp_cat;
    clean_up;
    ;;

  #### viewer modes

### main_display()
  dvi)
    case "${_OPT_DEVICE}" in
    ''|dvi) do_nothing; ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}"
      ;;
    esac;
    md_modefile="${md_modefile}".dvi;
    md_groggy="$(tmp_cat | grog -Tdvi)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_display;
    ;;
  html)
    case "${_OPT_DEVICE}" in
    ''|html) do_nothing; ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}";
      ;;
    esac;
    md_modefile="${md_modefile}".html;
    md_groggy="$(tmp_cat | grog -Thtml)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_display;
    ;;
### main_display()
  pdf)
    case "${_OPT_DEVICE}" in
    ''|ps)
      do_nothing;
      ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}";
      ;;
    esac;
    md_groggy="$(tmp_cat | grog -Tps)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_display _make_pdf;
    ;;
  ps)
    case "${_OPT_DEVICE}" in
    ''|ps)
      do_nothing;
      ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}";
      ;;
    esac;
    md_modefile="${md_modefile}".ps;
    md_groggy="$(tmp_cat | grog -Tps)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_display;
    ;;
### main_display()
  x)
    case "${_OPT_DEVICE}" in
    X*)
      md_device="${_OPT_DEVICE}"
      ;;
    *)
      case "${_OPT_RESOLUTION}" in
      100)
        md_device='X100';
        if obj _OPT_GEOMETRY is_empty
        then
          case "${_DISPLAY_PROG}" in
          gxditview|xditview)
            # add width of 800dpi for resolution of 100dpi to the args
            list_append _DISPLAY_ARGS '-geometry' '800';
            ;;
          esac;
        fi;
        ;;
      *)
        md_device='X75-12';
        ;;
      esac
    esac;
    md_groggy="$(tmp_cat | grog -T${md_device} -Z)";
    if is_not_equal "$?" 0
    then
      exit "${_ERROR}";
    fi;
    echo2 "grog output: ${md_groggy}";
    exit_test;
    _do_display;
    ;;
### main_display()
  X)
    case "${_OPT_DEVICE}" in
    '')
      md_groggy="$(tmp_cat | grog -X)";
      if is_not_equal "$?" 0
      then
        exit "${_ERROR}";
      fi;
      echo2 "grog output: ${md_groggy}";
      exit_test;
      ;;
    X*|dvi|html|lbp|lj4|ps)
      # these devices work with
      md_groggy="$(tmp_cat | grog -T"${_OPT_DEVICE}" -X)";
      if is_not_equal "$?" 0
      then
        exit "${_ERROR}";
      fi;
      echo2 "grog output: ${md_groggy}";
      exit_test;
      ;;
    *)
      warning "main_display(): \
wrong device for ${_DISPLAY_MODE} mode: ${_OPT_DEVICE}";
      md_groggy="$(tmp_cat | grog -Z)";
      if is_not_equal "$?" 0
      then
        exit "${_ERROR}";
      fi;
      echo2 "grog output: ${md_groggy}";
      exit_test;
      ;;
    esac;
    _do_display;
    ;;
  *)
    error "main_display(): unknown mode \`${_DISPLAY_MODE}'";
    ;;
  esac;
  eval ${_UNSET} md_addopts;
  eval ${_UNSET} md_device;
  eval ${_UNSET} md_groggy;
  eval ${_UNSET} md_modefile;
  eval ${_UNSET} md_p;
  eval ${_UNSET} md_pager;
  eval "${return_ok}";
} # main_display()


########################
# _do_display ([<prog>])
#
# Perform the generation of the output and view the result.  If an
# argument is given interpret it as a function name that is called in
# the midst (actually only for `pdf').
#
# Globals: $md_modefile, $md_groggy (from main_display())
#
_do_display()
{
  func_check _do_display '>=' 0 "$@";
  _do_opt_V;
  if obj _DISPLAY_PROG is_empty
  then
    trap_unset;
    {
      trap_set;
      eval "${md_groggy}" "${_ADDOPTS_GROFF}" "${_TMP_CAT}";
    } &
  else
    obj md_modefile rm_file;
    cat "${_TMP_CAT}" | \
      eval "${md_groggy}" "${_ADDOPTS_GROFF}" > "${md_modefile}";
    if obj md_modefile is_empty_file
    then
      echo2 '_do_display(): empty output.';
      clean_up;
      exit;
    fi;
    if is_not_empty "$1"
    then
      eval "$1";
    fi;
### _do_display() of main_display()
    obj _TMP_CAT rm_file_with_debug;
    if obj _OPT_STDOUT is_yes
    then
      cat "${md_modefile}";
      clean_up;
      exit;
    fi;
    if obj _VIEWER_BACKGROUND is_not_yes # for programs that run on tty
    then
      eval "'${_DISPLAY_PROG}'" ${_DISPLAY_ARGS} "\"${md_modefile}\"";
    else
      trap_unset;
      {
        trap_set;
        eval "${_DISPLAY_PROG}" ${_DISPLAY_ARGS} "\"${md_modefile}\"";
      } &
    fi;
  fi;
  eval "${return_ok}";
} # _do_display() of main_display()


#############
# _do_opt_V ()
#
# Check on option `-V'; if set print the corresponding output and leave.
#
# Globals: $_ALL_PARAMS, $_ADDOPTS_GROFF, $_DISPLAY_MODE, $_DISPLAY_PROG,
#          $_DISPLAY_ARGS, $md_groggy,  $md_modefile
#
# Variable prefix: _doV
#
_do_opt_V()
{
  func_check _do_opt_V '=' 0 "$@";
  if obj _OPT_V is_yes
  then
    _OPT_V='no';
    echo1 "Parameters:     ${_ALL_PARAMS}";
    echo1 "Display mode:   ${_DISPLAY_MODE}";
    echo1 "Output file:    ${md_modefile}";
    echo1 "Display prog:   ${_DISPLAY_PROG} ${_DISPLAY_ARGS}";
    a="$(eval echo1 "'${_ADDOPTS_GROFF}'")";
    exit_test;
    echo1 "Output of grog: ${md_groggy} $a";
    _doV_res="$(eval "${md_groggy}" "${_ADDOPTS_GROFF}")";
    exit_test;
    echo1 "groff -V:       ${_doV_res}"
    leave;
  fi;
  eval "${return_ok}";
} # _do_opt_V() of main_display()


##############
# _make_pdf ()
#
# Transform to pdf format; for pdf mode in _do_display().
#
# Globals: $md_modefile (from main_display())
#
# Variable prefix: _mp
#
_make_pdf()
{
  func_check _make_pdf '=' 0 "$@";
  _mp_psfile="${md_modefile}";
  md_modefile="${md_modefile}.pdf";
  obj md_modefile rm_file;
  if obj _PDF_HAS_PS2PDF is_yes && ps2pdf "${_mp_psfile}" "${md_modefile}";
  then
    :;
  elif obj _PDF_HAS_GS is_yes && gs -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
       -sOutputFile="${md_modefile}" -c save pop -f "${_mp_psfile}";
  then
    :;
  else
    _PDF_DID_NOT_WORK='yes';
    echo2 '_make_pdf(): Could not transform into pdf format. '\
'The Postscript mode (ps) is used instead.';
    _OPT_MODE='ps';
    main_set_mode;
    main_set_resources;
    main_display;
    exit;
  fi;
  obj _mp_psfile rm_file_with_debug;
  eval ${_UNSET} _mp_psfile;
  eval "${return_ok}";
} # _make_pdf() of main_display()


########################################################################
# main (<command_line_args>*)
#
# The main function for groffer.
#
# Arguments:
#
main()
{
  func_check main '>=' 0 "$@";
  # Do not change the sequence of the following functions!
  landmark '13: main_init()';
  main_init;
  landmark '14: main_parse_MANOPT()';
  main_parse_MANOPT;
  landmark '15: main_parse_args()';
  main_parse_args "$@";
  landmark '16: main_set_mode()';
  main_set_mode;
  landmark '17: main_do_fileargs()';
  main_do_fileargs;
  landmark '18: main_set_resources()';
  main_set_resources;
  landmark '19: main_display()';
  main_display;
  eval "${return_ok}";
}


########################################################################

main "$@";
