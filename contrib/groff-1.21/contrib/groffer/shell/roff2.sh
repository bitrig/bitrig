#! /bin/sh

# roff2* - transform roff files into other formats

# Source file position: <groff-source>/contrib/groffer/shell/roff2.sh
# Installed position: <prefix>/bin/roff2*

# Copyright (C) 2006, 2009 Free Software Foundation, Inc.
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


error_no_groffer='no';
error_no_groffer() {
  if test _"$error_no_groffer" = _yes
  then
    return;
  fi;
  error_no_groffer='yes';
  echo2 "$name: groffer is not available.";
}


error_no_options='no';
error_no_options() {
  if test _"$error_no_options" = _yes
  then
    return;
  fi;
  error_no_groffer='yes';
  echo2 "$name: groffer options are not allowed.";
}


usage_with_groffer() {
  cat <<EOF
usage: $Name [option]... [--] [filespec]...

-h | --help     print usage information
-v | --version  print version information

All other options are arbitrary options of "groffer"; the options
override the behavior of this program.

"filespec"s are the same as in "groffer": either the names of
existing, readable files or "-" for standard input or a search pattern
for man pages.  No "filespec" assumes standard input automatically.
EOF
}

usage_without_groffer() {
  cat <<EOF
usage: $Name [option]... [--] [filespec]...

-h | --help     print usage information
-v | --version  print version information

No other options are allowed because "groffer" is not available.

The only "filespec"s allowed are the names of existing, readable files
or "-" for standard input.  No "filespec" assumes standard input
automatically.
EOF
}


where_is_prog() {
  for p in `echo $PATH|sed "s/:/ /g"`
  do
    f="${p}/$1";
    if test -f "$f" && test -x "$f"
    then
      echo1 "$f";
      return;
    fi;
  done;
}


########################################################################

export NULL_DEV;
if test -c /dev/null
then
  NULL_DEV='/dev/null';
else
  NULL_DEV='NUL';
fi;

name="$(echo1 "$0" | sed 's|^.*//*\([^/]*\)$|\1|')";

case "$name" in
roff2[a-z]*)
  mode="$(echo1 "$name" | sed 's/^roff2//')";
  ;;
*)
  echo2 "wrong program name: $name";
  exit 1;
  ;;
esac;

groff_version="$(groff --version 2>$NULL_DEV)";
if test $? -gt 0
then
  echo2 "$name error: groff does not work.";
  exit 1;
fi;
groffer_version="$(groffer --version 2>$NULL_DEV)";
if test $? -gt 0
then
  has_groffer='no';
else
  has_groffer='yes';
fi;

if test _"${has_groffer}" = _yes
then
  for i
  do
    case $i in
    -v|--v|--ve|--ver|--vers|--versi|--versio|--version)
      echo1 "$name in $groffer_version";
      exit 0;
      ;;
    -h|--h|--he|--hel|--help)
      usage_with_groffer;
      exit 0;
      ;;
    esac;
  done;
  groffer --to-stdout --$mode "$@";
else				# not has_groffer
  reset=no;
  double_minus=no;
  for i
  do
    if test _"${reset}" = _no
    then
      set --;
      reset=yes;
    fi;
    if test _"${double_minus}" = _yes
    then
      set -- "$@" "$i";
      continue;
    fi;
    case "$i" in
    --)
      double_minus=yes;
      continue;
      ;;
    -)
      set -- "$@" '-';
      continue;
      ;;
    -v|--v|--ve|--ver|--vers|--versi|--versio|--version)
      echo1 "$name in $groff_version";
      exit 0;
      ;;
    -h|--h|--he|--hel|--help)
      usage_without_groffer;
      exit 0;
      ;;
    -*)
      error_no_groffer;
      error_no_options;
      ;;
    *)
      if test -f "$i" && test -r "$i"
      then
        set -- "$@" "$i";
      else
        error_no_groffer;
        echo2 "$i is not an existing, readable file.";
      fi;
      continue;
      ;;
    esac;
  done;				# for i

  if test $# -eq 0
  then
    set -- '-';
  fi;
  has_stdin=no;
  for i
  do
    case "$i" in
    -)
      has_stdin=yes;
      break;
      ;;
    esac;
  done;

  if test _$has_stdin = _yes
  then
    umask 0077;
    tempdir='';
    for d in "${GROFF_TMPDIR}" "${TMPDIR}" "${TMP}" "${TEMP}" \
           "${TEMPDIR}" "${HOME}"'/tmp' '/tmp' "${HOME}" '.'
    do
      if test _"$d" = _ || ! test -d "$d" || ! test -w "$d"
      then
        continue;
      fi;
      case "$d" in
      */)
        tempdir="$d";
        ;;
      *)
        tempdir="$d"'/';
        ;;
      esac;
    done;
    if test _$tempdir = _
    then
      echo2 "${name}: could not find a temporary directory."
      exit 1;
    fi;
    stdin=${tempdir}${name}_$$;
    if test -e "$stdin"
    then
      rm -f "$stdin";
      n=0;
      f="${stdin}_$n";
      while test -e "$f"
      do
	rm -f "$f";
        if ! test -e "$f"
        then
          break;
        fi;
        n="$(expr $n + 1)";
        f="${stdin}_$n";
      done;
      stdin="$f";
    fi;
    reset=no;
    for i
    do
      if test _"${reset}" = _no
      then
        set --;
        reset=yes;
      fi;
      case "$i" in
      -)
        set -- "$@" "$stdin";
        ;;
      *)
        set -- "$@" "$i";
        ;;
      esac;
    done;
    cat>"$stdin";
  fi;				# if has_stdin

  case "$mode" in
  x)
    groff_options='-TX75-12 -Z';
    ;;
  text)
    groff_options='-Tlatin1';
    ;;
  pdf)
    ps2pdf="$(where_is_prog ps2pdf)";
    if test _"$ps2pdf" = _
    then
      ps2pdf="$(where_is_prog gs)";
      if test _"$ps2pdf" = _
      then
        echo2 "$name: cannot transform to pdf format.";
        exit 1;
      fi;
      ps2pdf="$ps2pdf -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite '-sOutputFile=- -c save pop -f -";
    else
      ps2pdf="$ps2pdf -";
    fi;
    grog="$(grog -Tps "$@")";
    eval $grog | ${ps2pdf};
    exit $?;
    ;;
  *)
    groff_options="-T$mode";
    ;;
  esac;
  grog="$(grog $groff_options "$@")";
  eval $grog;
  exit $?;
fi;				# not has_groffer
