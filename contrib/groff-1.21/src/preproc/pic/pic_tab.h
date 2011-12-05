
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
     LABEL = 258,
     VARIABLE = 259,
     NUMBER = 260,
     TEXT = 261,
     COMMAND_LINE = 262,
     DELIMITED = 263,
     ORDINAL = 264,
     TH = 265,
     LEFT_ARROW_HEAD = 266,
     RIGHT_ARROW_HEAD = 267,
     DOUBLE_ARROW_HEAD = 268,
     LAST = 269,
     BOX = 270,
     CIRCLE = 271,
     ELLIPSE = 272,
     ARC = 273,
     LINE = 274,
     ARROW = 275,
     MOVE = 276,
     SPLINE = 277,
     HEIGHT = 278,
     RADIUS = 279,
     FIGNAME = 280,
     WIDTH = 281,
     DIAMETER = 282,
     UP = 283,
     DOWN = 284,
     RIGHT = 285,
     LEFT = 286,
     FROM = 287,
     TO = 288,
     AT = 289,
     WITH = 290,
     BY = 291,
     THEN = 292,
     SOLID = 293,
     DOTTED = 294,
     DASHED = 295,
     CHOP = 296,
     SAME = 297,
     INVISIBLE = 298,
     LJUST = 299,
     RJUST = 300,
     ABOVE = 301,
     BELOW = 302,
     OF = 303,
     THE = 304,
     WAY = 305,
     BETWEEN = 306,
     AND = 307,
     HERE = 308,
     DOT_N = 309,
     DOT_E = 310,
     DOT_W = 311,
     DOT_S = 312,
     DOT_NE = 313,
     DOT_SE = 314,
     DOT_NW = 315,
     DOT_SW = 316,
     DOT_C = 317,
     DOT_START = 318,
     DOT_END = 319,
     DOT_X = 320,
     DOT_Y = 321,
     DOT_HT = 322,
     DOT_WID = 323,
     DOT_RAD = 324,
     SIN = 325,
     COS = 326,
     ATAN2 = 327,
     LOG = 328,
     EXP = 329,
     SQRT = 330,
     K_MAX = 331,
     K_MIN = 332,
     INT = 333,
     RAND = 334,
     SRAND = 335,
     COPY = 336,
     THRU = 337,
     TOP = 338,
     BOTTOM = 339,
     UPPER = 340,
     LOWER = 341,
     SH = 342,
     PRINT = 343,
     CW = 344,
     CCW = 345,
     FOR = 346,
     DO = 347,
     IF = 348,
     ELSE = 349,
     ANDAND = 350,
     OROR = 351,
     NOTEQUAL = 352,
     EQUALEQUAL = 353,
     LESSEQUAL = 354,
     GREATEREQUAL = 355,
     LEFT_CORNER = 356,
     RIGHT_CORNER = 357,
     NORTH = 358,
     SOUTH = 359,
     EAST = 360,
     WEST = 361,
     CENTER = 362,
     END = 363,
     START = 364,
     RESET = 365,
     UNTIL = 366,
     PLOT = 367,
     THICKNESS = 368,
     FILL = 369,
     COLORED = 370,
     OUTLINED = 371,
     SHADED = 372,
     XSLANTED = 373,
     YSLANTED = 374,
     ALIGNED = 375,
     SPRINTF = 376,
     COMMAND = 377,
     DEFINE = 378,
     UNDEF = 379
   };
#endif
/* Tokens.  */
#define LABEL 258
#define VARIABLE 259
#define NUMBER 260
#define TEXT 261
#define COMMAND_LINE 262
#define DELIMITED 263
#define ORDINAL 264
#define TH 265
#define LEFT_ARROW_HEAD 266
#define RIGHT_ARROW_HEAD 267
#define DOUBLE_ARROW_HEAD 268
#define LAST 269
#define BOX 270
#define CIRCLE 271
#define ELLIPSE 272
#define ARC 273
#define LINE 274
#define ARROW 275
#define MOVE 276
#define SPLINE 277
#define HEIGHT 278
#define RADIUS 279
#define FIGNAME 280
#define WIDTH 281
#define DIAMETER 282
#define UP 283
#define DOWN 284
#define RIGHT 285
#define LEFT 286
#define FROM 287
#define TO 288
#define AT 289
#define WITH 290
#define BY 291
#define THEN 292
#define SOLID 293
#define DOTTED 294
#define DASHED 295
#define CHOP 296
#define SAME 297
#define INVISIBLE 298
#define LJUST 299
#define RJUST 300
#define ABOVE 301
#define BELOW 302
#define OF 303
#define THE 304
#define WAY 305
#define BETWEEN 306
#define AND 307
#define HERE 308
#define DOT_N 309
#define DOT_E 310
#define DOT_W 311
#define DOT_S 312
#define DOT_NE 313
#define DOT_SE 314
#define DOT_NW 315
#define DOT_SW 316
#define DOT_C 317
#define DOT_START 318
#define DOT_END 319
#define DOT_X 320
#define DOT_Y 321
#define DOT_HT 322
#define DOT_WID 323
#define DOT_RAD 324
#define SIN 325
#define COS 326
#define ATAN2 327
#define LOG 328
#define EXP 329
#define SQRT 330
#define K_MAX 331
#define K_MIN 332
#define INT 333
#define RAND 334
#define SRAND 335
#define COPY 336
#define THRU 337
#define TOP 338
#define BOTTOM 339
#define UPPER 340
#define LOWER 341
#define SH 342
#define PRINT 343
#define CW 344
#define CCW 345
#define FOR 346
#define DO 347
#define IF 348
#define ELSE 349
#define ANDAND 350
#define OROR 351
#define NOTEQUAL 352
#define EQUALEQUAL 353
#define LESSEQUAL 354
#define GREATEREQUAL 355
#define LEFT_CORNER 356
#define RIGHT_CORNER 357
#define NORTH 358
#define SOUTH 359
#define EAST 360
#define WEST 361
#define CENTER 362
#define END 363
#define START 364
#define RESET 365
#define UNTIL 366
#define PLOT 367
#define THICKNESS 368
#define FILL 369
#define COLORED 370
#define OUTLINED 371
#define SHADED 372
#define XSLANTED 373
#define YSLANTED 374
#define ALIGNED 375
#define SPRINTF 376
#define COMMAND 377
#define DEFINE 378
#define UNDEF 379




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 67 "pic.y"

	char *str;
	int n;
	double x;
	struct { double x, y; } pair;
	struct { double x; char *body; } if_data;
	struct { char *str; const char *filename; int lineno; } lstr;
	struct { double *v; int nv; int maxv; } dv;
	struct { double val; int is_multiplicative; } by;
	place pl;
	object *obj;
	corner crn;
	path *pth;
	object_spec *spec;
	saved_state *pstate;
	graphics_state state;
	object_type obtype;



/* Line 1676 of yacc.c  */
#line 321 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


