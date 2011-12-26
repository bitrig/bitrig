
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     OVER = 258,
     SMALLOVER = 259,
     SQRT = 260,
     SUB = 261,
     SUP = 262,
     LPILE = 263,
     RPILE = 264,
     CPILE = 265,
     PILE = 266,
     LEFT = 267,
     RIGHT = 268,
     TO = 269,
     FROM = 270,
     SIZE = 271,
     FONT = 272,
     ROMAN = 273,
     BOLD = 274,
     ITALIC = 275,
     FAT = 276,
     ACCENT = 277,
     BAR = 278,
     UNDER = 279,
     ABOVE = 280,
     TEXT = 281,
     QUOTED_TEXT = 282,
     FWD = 283,
     BACK = 284,
     DOWN = 285,
     UP = 286,
     MATRIX = 287,
     COL = 288,
     LCOL = 289,
     RCOL = 290,
     CCOL = 291,
     MARK = 292,
     LINEUP = 293,
     TYPE = 294,
     VCENTER = 295,
     PRIME = 296,
     SPLIT = 297,
     NOSPLIT = 298,
     UACCENT = 299,
     SPECIAL = 300,
     SPACE = 301,
     GFONT = 302,
     GSIZE = 303,
     DEFINE = 304,
     NDEFINE = 305,
     TDEFINE = 306,
     SDEFINE = 307,
     UNDEF = 308,
     IFDEF = 309,
     INCLUDE = 310,
     DELIM = 311,
     CHARTYPE = 312,
     SET = 313,
     GRFONT = 314,
     GBFONT = 315
   };
#endif
/* Tokens.  */
#define OVER 258
#define SMALLOVER 259
#define SQRT 260
#define SUB 261
#define SUP 262
#define LPILE 263
#define RPILE 264
#define CPILE 265
#define PILE 266
#define LEFT 267
#define RIGHT 268
#define TO 269
#define FROM 270
#define SIZE 271
#define FONT 272
#define ROMAN 273
#define BOLD 274
#define ITALIC 275
#define FAT 276
#define ACCENT 277
#define BAR 278
#define UNDER 279
#define ABOVE 280
#define TEXT 281
#define QUOTED_TEXT 282
#define FWD 283
#define BACK 284
#define DOWN 285
#define UP 286
#define MATRIX 287
#define COL 288
#define LCOL 289
#define RCOL 290
#define CCOL 291
#define MARK 292
#define LINEUP 293
#define TYPE 294
#define VCENTER 295
#define PRIME 296
#define SPLIT 297
#define NOSPLIT 298
#define UACCENT 299
#define SPECIAL 300
#define SPACE 301
#define GFONT 302
#define GSIZE 303
#define DEFINE 304
#define NDEFINE 305
#define TDEFINE 306
#define SDEFINE 307
#define UNDEF 308
#define IFDEF 309
#define INCLUDE 310
#define DELIM 311
#define CHARTYPE 312
#define SET 313
#define GRFONT 314
#define GBFONT 315




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 31 "eqn.y"

	char *str;
	box *b;
	pile_box *pb;
	matrix_box *mb;
	int n;
	column *col;



/* Line 1676 of yacc.c  */
#line 183 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


