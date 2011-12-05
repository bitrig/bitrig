/* Provide relocation for macro and font files.
   Copyright (C) 2005-2006, 2009 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */

#ifdef __cplusplus
extern char *curr_prefix;
extern size_t curr_prefix_len;

void set_current_prefix ();
char *xdirname (char *s);
char *searchpath (const char *name, const char *pathp);
#endif

/* This function has C linkage.  */
extern
#ifdef __cplusplus
"C"
#endif
char *relocatep (const char *path);

#ifdef __cplusplus
char *relocate (const char *path);
#endif
