#! /bin/sh
#
# Copyright (C) 1991, 2000, 2009
#   Free Software Foundation, Inc.
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
#
# gendef filename var=val var=val ...
#
# This script is used to generate src/include/defs.h.
#
file=$1
shift

defs="#define $1"
shift
for def
do
	defs="$defs
#define $def"
done

# Use $TMPDIR if defined.  Default to cwd, for non-Unix systems
# which don't have /tmp on each drive (we are going to remove
# the file before we exit anyway).  Put the PID in the basename,
# since the extension can only hold 3 characters on MS-DOS.
t=${TMPDIR-.}/gro$$.tmp

sed -e 's/=/ /' >$t <<EOF
$defs
EOF

test -r $file && cmp -s $t $file || cp $t $file

rm -f $t

exit 0

# eof
