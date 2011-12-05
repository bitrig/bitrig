// -*- C++ -*-
/* Copyright (C) 1989, 1990, 1991, 1992, 2001, 2002, 2006, 2009, 2010
   Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.com)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or
(at your option) any later version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>. */

#include <vector>
#include <utility>

extern int class_flag;	// set if there was a call to `.class'
extern void get_flags();

class macro;

class charinfo : glyph {
  static int next_index;
  charinfo *translation;
  macro *mac;
  unsigned char special_translation;
  unsigned char hyphenation_code;
  unsigned int flags;
  unsigned char ascii_code;
  unsigned char asciify_code;
  char not_found;
  char transparent_translate;	// non-zero means translation applies
				// to transparent throughput
  char translate_input;		// non-zero means that asciify_code is
				// active for .asciify (set by .trin)
  char_mode mode;
  // Unicode character classes
  std::vector<std::pair<int, int> > ranges;
  std::vector<charinfo *> nested_classes;
public:
  enum {		// Values for the flags bitmask.  See groff
			// manual, description of the `.cflags' request.
    ENDS_SENTENCE = 0x01,
    BREAK_BEFORE = 0x02,
    BREAK_AFTER = 0x04,
    OVERLAPS_HORIZONTALLY = 0x08,
    OVERLAPS_VERTICALLY = 0x10,
    TRANSPARENT = 0x20,
    IGNORE_HCODES = 0x40,
    DONT_BREAK_BEFORE = 0x80,
    DONT_BREAK_AFTER = 0x100,
    INTER_CHAR_SPACE = 0x200
  };
  enum {
    TRANSLATE_NONE,
    TRANSLATE_SPACE,
    TRANSLATE_DUMMY,
    TRANSLATE_STRETCHABLE_SPACE,
    TRANSLATE_HYPHEN_INDICATOR
  };
  symbol nm;
  charinfo(symbol);
  glyph *as_glyph();
  int ends_sentence();
  int overlaps_vertically();
  int overlaps_horizontally();
  int can_break_before();
  int can_break_after();
  int transparent();
  int ignore_hcodes();
  int prohibit_break_before();
  int prohibit_break_after();
  int inter_char_space();
  unsigned char get_hyphenation_code();
  unsigned char get_ascii_code();
  unsigned char get_asciify_code();
  int get_unicode_code();
  void set_hyphenation_code(unsigned char);
  void set_ascii_code(unsigned char);
  void set_asciify_code(unsigned char);
  void set_translation_input();
  int get_translation_input();
  charinfo *get_translation(int = 0);
  void set_translation(charinfo *, int, int);
  void get_flags();
  void set_flags(unsigned int);
  void set_special_translation(int, int);
  int get_special_translation(int = 0);
  macro *set_macro(macro *);
  macro *setx_macro(macro *, char_mode);
  macro *get_macro();
  int first_time_not_found();
  void set_number(int);
  int get_number();
  int numbered();
  int is_normal();
  int is_fallback();
  int is_special();
  symbol *get_symbol();
  void add_to_class(int);
  void add_to_class(int, int);
  void add_to_class(charinfo *);
  bool is_class();
  bool contains(int, bool = false);
  bool contains(symbol, bool = false);
  bool contains(charinfo *, bool = false);
};

charinfo *get_charinfo(symbol);
extern charinfo *charset_table[];
charinfo *get_charinfo_by_number(int);

inline int charinfo::overlaps_horizontally()
{
  if (class_flag)
    ::get_flags();
  return flags & OVERLAPS_HORIZONTALLY;
}

inline int charinfo::overlaps_vertically()
{
  if (class_flag)
    ::get_flags();
  return flags & OVERLAPS_VERTICALLY;
}

inline int charinfo::can_break_before()
{
  if (class_flag)
    ::get_flags();
  return flags & BREAK_BEFORE;
}

inline int charinfo::can_break_after()
{
  if (class_flag)
    ::get_flags();
  return flags & BREAK_AFTER;
}

inline int charinfo::ends_sentence()
{
  if (class_flag)
    ::get_flags();
  return flags & ENDS_SENTENCE;
}

inline int charinfo::transparent()
{
  if (class_flag)
    ::get_flags();
  return flags & TRANSPARENT;
}

inline int charinfo::ignore_hcodes()
{
  if (class_flag)
    ::get_flags();
  return flags & IGNORE_HCODES;
}

inline int charinfo::prohibit_break_before()
{
  if (class_flag)
    ::get_flags();
  return flags & DONT_BREAK_BEFORE;
}

inline int charinfo::prohibit_break_after()
{
  if (class_flag)
    ::get_flags();
  return flags & DONT_BREAK_AFTER;
}

inline int charinfo::inter_char_space()
{
  if (class_flag)
    ::get_flags();
  return flags & INTER_CHAR_SPACE;
}

inline int charinfo::numbered()
{
  return number >= 0;
}

inline int charinfo::is_normal()
{
  return mode == CHAR_NORMAL;
}

inline int charinfo::is_fallback()
{
  return mode == CHAR_FALLBACK;
}

inline int charinfo::is_special()
{
  return mode == CHAR_SPECIAL;
}

inline charinfo *charinfo::get_translation(int transparent_throughput)
{
  return (transparent_throughput && !transparent_translate
	  ? 0
	  : translation);
}

inline unsigned char charinfo::get_hyphenation_code()
{
  return hyphenation_code;
}

inline unsigned char charinfo::get_ascii_code()
{
  return ascii_code;
}

inline unsigned char charinfo::get_asciify_code()
{
  return (translate_input ? asciify_code : 0);
}

inline void charinfo::set_flags(unsigned int c)
{
  flags = c;
}

inline glyph *charinfo::as_glyph()
{
  return this;
}

inline void charinfo::set_translation_input()
{
  translate_input = 1;
}

inline int charinfo::get_translation_input()
{
  return translate_input;
}

inline int charinfo::get_special_translation(int transparent_throughput)
{
  return (transparent_throughput && !transparent_translate
	  ? int(TRANSLATE_NONE)
	  : special_translation);
}

inline macro *charinfo::get_macro()
{
  return mac;
}

inline int charinfo::first_time_not_found()
{
  if (not_found)
    return 0;
  else {
    not_found = 1;
    return 1;
  }
}

inline symbol *charinfo::get_symbol()
{
  return &nm;
}

inline void charinfo::add_to_class(int c)
{
  class_flag = 1;
  // TODO ranges cumbersome for single characters?
  ranges.push_back(std::pair<int, int>(c, c));
}

inline void charinfo::add_to_class(int lo,
				   int hi)
{
  class_flag = 1;
  ranges.push_back(std::pair<int, int>(lo, hi));
}

inline void charinfo::add_to_class(charinfo *ci)
{
  class_flag = 1;
  nested_classes.push_back(ci);
}

inline bool charinfo::is_class()
{
  return (!ranges.empty() || !nested_classes.empty());
}
