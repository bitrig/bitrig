
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 21 "pic.y"

#include "pic.h"
#include "ptable.h"
#include "object.h"

extern int delim_flag;
extern void copy_rest_thru(const char *, const char *);
extern void copy_file_thru(const char *, const char *, const char *);
extern void push_body(const char *);
extern void do_for(char *var, double from, double to,
		   int by_is_multiplicative, double by, char *body);
extern void do_lookahead();

/* Maximum number of characters produced by printf("%g") */
#define GDIGITS 14

int yylex();
void yyerror(const char *);

void reset(const char *nm);
void reset_all();

place *lookup_label(const char *);
void define_label(const char *label, const place *pl);

direction current_direction;
position current_position;

implement_ptable(place)

PTABLE(place) top_table;

PTABLE(place) *current_table = &top_table;
saved_state *current_saved_state = 0;

object_list olist;

const char *ordinal_postfix(int n);
const char *object_type_name(object_type type);
char *format_number(const char *form, double n);
char *do_sprintf(const char *form, const double *v, int nv);



/* Line 189 of yacc.c  */
#line 118 "pic.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
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



/* Line 214 of yacc.c  */
#line 423 "pic.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 435 "pic.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2438

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  146
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  49
/* YYNRULES -- Number of rules.  */
#define YYNRULES  260
/* YYNRULES -- Number of states.  */
#define YYNSTATES  454

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   379

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,     2,     2,     2,   136,     2,     2,
     126,   145,   134,   132,   129,   133,   125,   135,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   141,   139,
     130,   140,   131,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   128,     2,   144,   138,     2,   127,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   142,     2,   143,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    13,    17,    18,    20,
      22,    25,    29,    33,    38,    40,    42,    44,    46,    48,
      51,    54,    55,    59,    62,    63,    64,    72,    73,    74,
      81,    82,    93,    95,    96,   101,   103,   105,   107,   109,
     112,   115,   119,   121,   124,   126,   128,   130,   131,   137,
     138,   141,   143,   145,   149,   153,   157,   161,   165,   169,
     173,   177,   180,   181,   184,   188,   190,   195,   200,   205,
     206,   207,   214,   216,   217,   219,   221,   223,   225,   227,
     229,   231,   233,   235,   237,   240,   244,   245,   250,   254,
     258,   262,   266,   269,   272,   276,   279,   283,   286,   290,
     293,   297,   301,   305,   309,   313,   317,   321,   324,   327,
     330,   334,   337,   341,   344,   348,   352,   356,   360,   364,
     368,   371,   375,   378,   381,   384,   387,   390,   393,   396,
     399,   402,   405,   408,   411,   415,   418,   420,   426,   427,
     431,   433,   435,   439,   441,   445,   451,   455,   461,   467,
     473,   481,   488,   497,   499,   504,   508,   512,   514,   517,
     520,   524,   526,   528,   530,   534,   536,   540,   542,   545,
     548,   551,   553,   555,   557,   559,   561,   563,   565,   568,
     570,   573,   577,   579,   581,   584,   586,   592,   597,   601,
     605,   608,   610,   612,   614,   616,   618,   620,   622,   624,
     626,   628,   630,   632,   634,   636,   638,   641,   644,   647,
     650,   652,   654,   657,   660,   663,   666,   668,   670,   672,
     674,   676,   678,   680,   682,   684,   688,   690,   692,   695,
     698,   701,   704,   707,   711,   715,   719,   723,   727,   731,
     734,   738,   743,   748,   755,   760,   765,   770,   777,   784,
     789,   794,   798,   803,   807,   811,   815,   819,   823,   827,
     831
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     147,     0,    -1,   150,    -1,   148,    -1,   150,   149,   150,
      -1,   170,    -1,   149,   151,   170,    -1,    -1,   151,    -1,
     139,    -1,   151,   139,    -1,    25,   140,   160,    -1,     4,
     140,   167,    -1,     4,   141,   140,   167,    -1,    28,    -1,
      29,    -1,    31,    -1,    30,    -1,     7,    -1,   122,   162,
      -1,    88,   162,    -1,    -1,    87,   153,     8,    -1,    81,
       6,    -1,    -1,    -1,    81,     6,    82,   154,     8,   155,
     166,    -1,    -1,    -1,    81,    82,   156,     8,   157,   166,
      -1,    -1,    91,     4,   140,   192,    33,   192,   169,    92,
     158,     8,    -1,   164,    -1,    -1,   164,    94,   159,     8,
      -1,   161,    -1,   110,    -1,     4,    -1,     3,    -1,   110,
       4,    -1,   161,     4,    -1,   161,   129,     4,    -1,   163,
      -1,   162,   163,    -1,   192,    -1,   176,    -1,   178,    -1,
      -1,    93,   167,    37,   165,     8,    -1,    -1,   111,     6,
      -1,   192,    -1,   168,    -1,   176,    98,   176,    -1,   176,
      97,   176,    -1,   168,    95,   168,    -1,   168,    95,   192,
      -1,   192,    95,   168,    -1,   168,    96,   168,    -1,   168,
      96,   192,    -1,   192,    96,   168,    -1,   137,   168,    -1,
      -1,    36,   192,    -1,    36,   134,   192,    -1,   174,    -1,
       3,   141,   150,   170,    -1,     3,   141,   150,   179,    -1,
       3,   141,   150,   182,    -1,    -1,    -1,   142,   171,   148,
     143,   172,   173,    -1,   152,    -1,    -1,   170,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,   176,    -1,   112,   192,    -1,
     112,   192,   176,    -1,    -1,   128,   175,   148,   144,    -1,
     174,    23,   192,    -1,   174,    24,   192,    -1,   174,    26,
     192,    -1,   174,    27,   192,    -1,   174,   192,    -1,   174,
      28,    -1,   174,    28,   192,    -1,   174,    29,    -1,   174,
      29,   192,    -1,   174,    30,    -1,   174,    30,   192,    -1,
     174,    31,    -1,   174,    31,   192,    -1,   174,    32,   178,
      -1,   174,    33,   178,    -1,   174,    34,   178,    -1,   174,
      35,   190,    -1,   174,    35,   178,    -1,   174,    36,   181,
      -1,   174,    37,    -1,   174,    38,    -1,   174,    39,    -1,
     174,    39,   192,    -1,   174,    40,    -1,   174,    40,   192,
      -1,   174,   114,    -1,   174,   114,   192,    -1,   174,   118,
     192,    -1,   174,   119,   192,    -1,   174,   117,   176,    -1,
     174,   115,   176,    -1,   174,   116,   176,    -1,   174,    41,
      -1,   174,    41,   192,    -1,   174,    42,    -1,   174,    43,
      -1,   174,    11,    -1,   174,    12,    -1,   174,    13,    -1,
     174,    89,    -1,   174,    90,    -1,   174,   176,    -1,   174,
      44,    -1,   174,    45,    -1,   174,    46,    -1,   174,    47,
      -1,   174,   113,   192,    -1,   174,   120,    -1,     6,    -1,
     121,   126,     6,   177,   145,    -1,    -1,   177,   129,   192,
      -1,   179,    -1,   182,    -1,   126,   182,   145,    -1,   181,
      -1,   178,   132,   181,    -1,   126,   178,   132,   181,   145,
      -1,   178,   133,   181,    -1,   126,   178,   133,   181,   145,
      -1,   126,   178,   129,   178,   145,    -1,   192,   180,   178,
      52,   178,    -1,   126,   192,   180,   178,    52,   178,   145,
      -1,   194,   130,   178,   129,   178,   131,    -1,   126,   194,
     130,   178,   129,   178,   131,   145,    -1,    51,    -1,    48,
      49,    50,    51,    -1,   192,   129,   192,    -1,   126,   181,
     145,    -1,   183,    -1,   183,   191,    -1,   191,   183,    -1,
     191,    48,   183,    -1,    53,    -1,     3,    -1,   186,    -1,
     183,   125,     3,    -1,     9,    -1,   127,   167,    10,    -1,
      14,    -1,   184,    14,    -1,   184,   187,    -1,   185,   187,
      -1,    15,    -1,    16,    -1,    17,    -1,    18,    -1,    19,
      -1,    20,    -1,    22,    -1,   128,   144,    -1,     6,    -1,
     125,     3,    -1,   188,   125,     3,    -1,   191,    -1,   188,
      -1,   188,   191,    -1,   189,    -1,   126,   189,   129,   189,
     145,    -1,     9,    14,   187,   189,    -1,    14,   187,   189,
      -1,     9,   187,   189,    -1,     3,   189,    -1,    54,    -1,
      55,    -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,
      60,    -1,    61,    -1,    62,    -1,    63,    -1,    64,    -1,
      83,    -1,    84,    -1,    31,    -1,    30,    -1,    85,    31,
      -1,    86,    31,    -1,    85,    30,    -1,    86,    30,    -1,
     101,    -1,   102,    -1,    85,   101,    -1,    86,   101,    -1,
      85,   102,    -1,    86,   102,    -1,   103,    -1,   104,    -1,
     105,    -1,   106,    -1,   107,    -1,   109,    -1,   108,    -1,
     193,    -1,   194,    -1,   192,   130,   192,    -1,     4,    -1,
       5,    -1,   182,    65,    -1,   182,    66,    -1,   182,    67,
      -1,   182,    68,    -1,   182,    69,    -1,   192,   132,   192,
      -1,   192,   133,   192,    -1,   192,   134,   192,    -1,   192,
     135,   192,    -1,   192,   136,   192,    -1,   192,   138,   192,
      -1,   133,   192,    -1,   126,   167,   145,    -1,    70,   126,
     167,   145,    -1,    71,   126,   167,   145,    -1,    72,   126,
     167,   129,   167,   145,    -1,    73,   126,   167,   145,    -1,
      74,   126,   167,   145,    -1,    75,   126,   167,   145,    -1,
      76,   126,   167,   129,   167,   145,    -1,    77,   126,   167,
     129,   167,   145,    -1,    78,   126,   167,   145,    -1,    79,
     126,   167,   145,    -1,    79,   126,   145,    -1,    80,   126,
     167,   145,    -1,   192,    99,   192,    -1,   192,   131,   192,
      -1,   192,   100,   192,    -1,   192,    98,   192,    -1,   192,
      97,   192,    -1,   192,    95,   192,    -1,   192,    96,   192,
      -1,   137,   192,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   277,   277,   278,   287,   292,   294,   298,   300,   304,
     305,   309,   317,   322,   334,   336,   338,   340,   342,   347,
     352,   359,   358,   369,   377,   379,   376,   390,   392,   389,
     402,   401,   410,   419,   418,   432,   433,   438,   439,   443,
     448,   453,   461,   463,   482,   489,   491,   502,   501,   513,
     514,   519,   521,   526,   532,   538,   540,   542,   544,   546,
     548,   550,   557,   561,   566,   574,   588,   594,   602,   609,
     615,   608,   624,   634,   635,   640,   642,   644,   646,   651,
     658,   665,   672,   679,   684,   689,   697,   696,   723,   729,
     735,   741,   747,   766,   773,   780,   787,   794,   801,   808,
     815,   822,   829,   844,   856,   862,   871,   878,   903,   907,
     913,   919,   925,   931,   936,   942,   948,   954,   961,   970,
     977,   993,  1010,  1015,  1020,  1025,  1030,  1035,  1040,  1045,
    1053,  1063,  1073,  1083,  1093,  1099,  1107,  1109,  1121,  1126,
    1156,  1158,  1164,  1173,  1175,  1180,  1185,  1190,  1195,  1200,
    1205,  1211,  1216,  1224,  1225,  1229,  1234,  1240,  1242,  1248,
    1254,  1260,  1269,  1279,  1281,  1290,  1292,  1300,  1302,  1307,
    1322,  1340,  1342,  1344,  1346,  1348,  1350,  1352,  1354,  1356,
    1361,  1363,  1371,  1375,  1377,  1385,  1387,  1393,  1399,  1405,
    1411,  1420,  1422,  1424,  1426,  1428,  1430,  1432,  1434,  1436,
    1438,  1440,  1442,  1444,  1446,  1448,  1450,  1452,  1454,  1456,
    1458,  1460,  1462,  1464,  1466,  1468,  1470,  1472,  1474,  1476,
    1478,  1480,  1482,  1487,  1489,  1494,  1499,  1507,  1509,  1516,
    1523,  1530,  1537,  1544,  1546,  1548,  1550,  1558,  1566,  1579,
    1581,  1583,  1592,  1601,  1614,  1623,  1632,  1641,  1643,  1645,
    1647,  1649,  1655,  1660,  1662,  1664,  1666,  1668,  1670,  1672,
    1674
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LABEL", "VARIABLE", "NUMBER", "TEXT",
  "COMMAND_LINE", "DELIMITED", "ORDINAL", "TH", "LEFT_ARROW_HEAD",
  "RIGHT_ARROW_HEAD", "DOUBLE_ARROW_HEAD", "LAST", "BOX", "CIRCLE",
  "ELLIPSE", "ARC", "LINE", "ARROW", "MOVE", "SPLINE", "HEIGHT", "RADIUS",
  "FIGNAME", "WIDTH", "DIAMETER", "UP", "DOWN", "RIGHT", "LEFT", "FROM",
  "TO", "AT", "WITH", "BY", "THEN", "SOLID", "DOTTED", "DASHED", "CHOP",
  "SAME", "INVISIBLE", "LJUST", "RJUST", "ABOVE", "BELOW", "OF", "THE",
  "WAY", "BETWEEN", "AND", "HERE", "DOT_N", "DOT_E", "DOT_W", "DOT_S",
  "DOT_NE", "DOT_SE", "DOT_NW", "DOT_SW", "DOT_C", "DOT_START", "DOT_END",
  "DOT_X", "DOT_Y", "DOT_HT", "DOT_WID", "DOT_RAD", "SIN", "COS", "ATAN2",
  "LOG", "EXP", "SQRT", "K_MAX", "K_MIN", "INT", "RAND", "SRAND", "COPY",
  "THRU", "TOP", "BOTTOM", "UPPER", "LOWER", "SH", "PRINT", "CW", "CCW",
  "FOR", "DO", "IF", "ELSE", "ANDAND", "OROR", "NOTEQUAL", "EQUALEQUAL",
  "LESSEQUAL", "GREATEREQUAL", "LEFT_CORNER", "RIGHT_CORNER", "NORTH",
  "SOUTH", "EAST", "WEST", "CENTER", "END", "START", "RESET", "UNTIL",
  "PLOT", "THICKNESS", "FILL", "COLORED", "OUTLINED", "SHADED", "XSLANTED",
  "YSLANTED", "ALIGNED", "SPRINTF", "COMMAND", "DEFINE", "UNDEF", "'.'",
  "'('", "'`'", "'['", "','", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'",
  "'%'", "'!'", "'^'", "';'", "'='", "':'", "'{'", "'}'", "']'", "')'",
  "$accept", "top", "element_list", "middle_element_list",
  "optional_separator", "separator", "placeless_element", "$@1", "$@2",
  "$@3", "$@4", "$@5", "$@6", "$@7", "macro_name", "reset_variables",
  "print_args", "print_arg", "simple_if", "$@8", "until", "any_expr",
  "text_expr", "optional_by", "element", "@9", "$@10", "optional_element",
  "object_spec", "@11", "text", "sprintf_args", "position",
  "position_not_place", "between", "expr_pair", "place", "label",
  "ordinal", "optional_ordinal_last", "nth_primitive", "object_type",
  "label_path", "relative_path", "path", "corner", "expr",
  "expr_lower_than", "expr_not_lower_than", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,    46,    40,    96,    91,    44,
      60,    62,    43,    45,    42,    47,    37,    33,    94,    59,
      61,    58,   123,   125,    93,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   146,   147,   147,   148,   149,   149,   150,   150,   151,
     151,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   153,   152,   152,   154,   155,   152,   156,   157,   152,
     158,   152,   152,   159,   152,   152,   152,   160,   160,   161,
     161,   161,   162,   162,   163,   163,   163,   165,   164,   166,
     166,   167,   167,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   169,   169,   169,   170,   170,   170,   170,   171,
     172,   170,   170,   173,   173,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   175,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   176,   176,   177,   177,
     178,   178,   178,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   180,   180,   181,   181,   182,   182,   182,
     182,   182,   183,   183,   183,   184,   184,   185,   185,   186,
     186,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     188,   188,   189,   189,   189,   190,   190,   190,   190,   190,
     190,   191,   191,   191,   191,   191,   191,   191,   191,   191,
     191,   191,   191,   191,   191,   191,   191,   191,   191,   191,
     191,   191,   191,   191,   191,   191,   191,   191,   191,   191,
     191,   191,   191,   192,   192,   193,   194,   194,   194,   194,
     194,   194,   194,   194,   194,   194,   194,   194,   194,   194,
     194,   194,   194,   194,   194,   194,   194,   194,   194,   194,
     194,   194,   194,   194,   194,   194,   194,   194,   194,   194,
     194
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     3,     1,     3,     0,     1,     1,
       2,     3,     3,     4,     1,     1,     1,     1,     1,     2,
       2,     0,     3,     2,     0,     0,     7,     0,     0,     6,
       0,    10,     1,     0,     4,     1,     1,     1,     1,     2,
       2,     3,     1,     2,     1,     1,     1,     0,     5,     0,
       2,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     0,     2,     3,     1,     4,     4,     4,     0,
       0,     6,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     0,     4,     3,     3,
       3,     3,     2,     2,     3,     2,     3,     2,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       3,     2,     3,     2,     3,     3,     3,     3,     3,     3,
       2,     3,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     2,     1,     5,     0,     3,
       1,     1,     3,     1,     3,     5,     3,     5,     5,     5,
       7,     6,     8,     1,     4,     3,     3,     1,     2,     2,
       3,     1,     1,     1,     3,     1,     3,     1,     2,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       2,     3,     1,     1,     2,     1,     5,     4,     3,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       1,     1,     2,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     1,     2,     2,
       2,     2,     2,     3,     3,     3,     3,     3,     3,     2,
       3,     4,     4,     6,     4,     4,     4,     6,     6,     4,
       4,     3,     4,     3,     3,     3,     3,     3,     3,     3,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       7,     9,     0,     3,     2,     8,     1,     0,     0,   136,
      18,    75,    76,    77,    78,    79,    80,    81,    82,     0,
      14,    15,    17,    16,     0,    21,     0,     0,     0,    36,
       0,     0,     0,    86,    69,     7,    72,    35,    32,     5,
      65,    83,    10,     7,     0,     0,     0,    23,    27,     0,
     162,   226,   227,   165,   167,   205,   204,   161,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,   203,     0,     0,   210,   211,   216,   217,   218,   219,
     220,   222,   221,     0,     0,     0,     0,    20,    42,    45,
      46,   140,   143,   141,   157,     0,     0,   163,     0,    44,
     223,   224,     0,     0,     0,     0,    52,     0,     0,    51,
     224,    39,    84,     0,    19,     7,     7,     4,     8,    40,
       0,    33,   124,   125,   126,     0,     0,     0,     0,    93,
      95,    97,    99,     0,     0,     0,     0,     0,   107,   108,
     109,   111,   120,   122,   123,   130,   131,   132,   133,   127,
     128,     0,   113,     0,     0,     0,     0,     0,   135,   129,
      92,     0,    12,     0,    38,    37,    11,    24,     0,    22,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   208,   206,   212,   214,   209,   207,   213,   215,     0,
       0,   143,   141,    51,   224,     0,   239,   260,    43,     0,
       0,   228,   229,   230,   231,   232,     0,   158,   179,   168,
     171,   172,   173,   174,   175,   176,   177,     0,   169,   170,
       0,   159,     0,   153,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    61,   260,    47,     0,     0,     0,     0,     0,
       0,    85,   138,     0,     0,     0,     6,    41,     0,    88,
      89,    90,    91,    94,    96,    98,   100,   101,     0,   102,
     103,   162,   165,   167,     0,     0,   105,   183,   185,   104,
     182,     0,   106,     0,   110,   112,   121,   134,   114,   118,
     119,   117,   115,   116,   162,   226,   205,   204,    66,     0,
      67,    68,    13,     0,    28,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   251,     0,     0,   240,     0,     0,
       0,   156,   142,     0,     0,   166,   144,   146,   164,   178,
     160,     0,   258,   259,   257,   256,   253,   255,   155,   225,
     254,   233,   234,   235,   236,   237,   238,     0,     0,     0,
       0,    55,    56,    58,    59,    54,    53,    57,   258,    60,
     259,     0,    87,    70,    34,   190,   182,     0,     0,   180,
       0,     0,   184,     0,    51,    25,    49,   241,   242,     0,
     244,   245,   246,     0,     0,   249,   250,   252,     0,   144,
     146,     0,     0,     0,     0,     0,     0,    48,     0,   137,
      73,   189,   188,     0,   181,    49,     0,    29,     0,     0,
       0,   148,   145,   147,     0,     0,   154,   149,     0,    62,
     139,    74,    71,     0,    26,    50,   243,   247,   248,   149,
       0,   151,     0,     0,   186,   150,   151,     0,    63,    30,
     152,    64,     0,    31
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    35,   264,     5,    36,    49,   313,   415,
     178,   386,   452,   268,   176,    37,    97,    98,    38,   360,
     417,   199,   116,   443,    39,   126,   410,   432,    40,   125,
     117,   371,   100,   101,   249,   102,   118,   104,   105,   106,
     107,   228,   287,   288,   289,   108,   119,   110,   120
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -240
static const yytype_int16 yypact[] =
{
    -114,  -240,    20,  -240,   757,  -107,  -240,   -98,  -123,  -240,
    -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -106,
    -240,  -240,  -240,  -240,     9,  -240,  1087,    46,  1172,    49,
    1597,   -70,  1087,  -240,  -240,  -114,  -240,     3,   -33,  -240,
     877,  -240,  -240,  -114,  1172,   -60,    36,   -14,  -240,    74,
    -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,
    -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,   -34,
     -18,     8,    38,    47,    51,    65,   101,   102,   112,   122,
    -240,  -240,    21,   150,  -240,  -240,  -240,  -240,  -240,  -240,
    -240,  -240,  -240,  1257,  1172,  1597,  1597,  1087,  -240,  -240,
     -43,  -240,  -240,   357,  2242,    59,   258,  -240,    10,  2147,
    -240,     1,     6,  1172,  1172,   145,    -1,     2,   357,  2273,
    -240,  -240,   220,   249,  1087,  -114,  -114,  -240,   721,  -240,
     252,  -240,  -240,  -240,  -240,  1597,  1597,  1597,  1597,  2024,
    2024,  1853,  1939,  1682,  1682,  1682,  1427,  1767,  -240,  -240,
    2024,  2024,  2024,  -240,  -240,  -240,  -240,  -240,  -240,  -240,
    -240,  1597,  2024,    23,    23,    23,  1597,  1597,  -240,  -240,
    2282,   593,  -240,  1172,  -240,  -240,  -240,  -240,   250,  -240,
    1172,  1172,  1172,  1172,  1172,  1172,  1172,  1172,  1172,   458,
    1172,  -240,  -240,  -240,  -240,  -240,  -240,  -240,  -240,   121,
     107,   123,   256,  2157,   137,   261,   134,   134,  -240,  1767,
    1767,  -240,  -240,  -240,  -240,  -240,   276,  -240,  -240,  -240,
    -240,  -240,  -240,  -240,  -240,  -240,  -240,   138,  -240,  -240,
      24,   156,   235,  -240,  1597,  1597,  1597,  1597,  1597,  1597,
    1597,  1597,  1597,  1597,  1597,  1597,  1597,  1597,  1597,  1682,
    1682,  1597,  -240,   134,  -240,  1172,  1172,    23,    23,  1172,
    1172,  -240,  -240,   143,   757,   153,  -240,  -240,   280,  2282,
    2282,  2282,  2282,  2282,  2282,  2282,  2282,   -43,  2147,   -43,
     -43,  2253,   275,   275,   295,  1002,   -43,  2081,  -240,  -240,
      10,  1342,  -240,   694,  2282,  2282,  2282,  2282,  2282,  -240,
    -240,  -240,  2282,  2282,   -98,  -123,    16,    28,  -240,   -43,
      56,   302,  -240,   291,  -240,   155,   160,   172,   161,   164,
     167,   184,   185,   181,  -240,   186,   188,  -240,  1682,  1767,
    1767,  -240,  -240,  1682,  1682,  -240,  -240,  -240,  -240,  -240,
     156,   279,   314,  2291,   440,   440,   413,   413,  2282,   413,
     413,   -72,   -72,   134,   134,   134,   134,   -49,   117,   343,
     322,  -240,   314,   239,  2300,  -240,  -240,  -240,   314,   239,
    2300,  -119,  -240,  -240,  -240,  -240,  -240,  2116,  2116,  -240,
     206,   333,  -240,   123,  2131,  -240,   228,  -240,  -240,  1172,
    -240,  -240,  -240,  1172,  1172,  -240,  -240,  -240,  -110,   195,
     197,   -47,   128,   292,  1682,  1682,  1597,  -240,  1597,  -240,
     757,  -240,  -240,  2116,  -240,   228,   338,  -240,   200,   202,
     212,  -240,  -240,  -240,  1682,  1682,  -240,   -43,   -27,   360,
    2282,  -240,  -240,   214,  -240,  -240,  -240,  -240,  -240,   -73,
      30,  -240,  1512,   268,  -240,  -240,   216,  1597,  2282,  -240,
    -240,  2282,   354,  -240
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -240,  -240,    17,  -240,    12,   329,  -240,  -240,  -240,  -240,
    -240,  -240,  -240,  -240,  -240,  -240,   334,   -76,  -240,  -240,
     -42,    13,  -103,  -240,  -127,  -240,  -240,  -240,  -240,  -240,
       5,  -240,    99,   194,   169,   -44,     4,  -100,  -240,  -240,
    -240,  -104,  -240,  -239,  -240,   -50,   -26,  -240,    61
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -206
static const yytype_int16 yytable[] =
{
     109,   266,   229,   404,   122,   424,   109,   129,   231,    41,
     408,   252,     4,    50,   170,    47,   -17,    44,    45,    53,
       6,   208,   209,   210,    54,     1,   409,    50,   -16,     9,
     103,    99,    42,    53,    46,   421,   103,    99,    54,   174,
     175,   115,   375,    43,   308,   169,   380,   127,   208,   201,
     112,   191,   192,   121,   217,   171,   123,   172,   230,   209,
     210,   131,   245,   246,   247,   218,   248,   203,   177,   206,
     207,   109,   445,   219,   220,   221,   222,   223,   224,   225,
     173,   226,   179,   209,   210,   209,   210,   111,   253,   209,
     210,    48,   180,   111,   255,   256,   290,   202,   109,   257,
     258,   103,    99,   292,   441,   209,   210,   205,   181,   269,
     270,   271,   272,   273,   274,   275,   276,   278,   278,   278,
     278,   293,   193,   194,   294,   295,   296,   261,   103,    99,
     340,   250,   130,    41,   182,   297,   298,    94,   411,   412,
     302,   303,   263,   265,    31,   278,   251,   103,   103,   103,
     103,    94,   361,   363,   204,   -17,   367,   369,   111,   -17,
     -17,   446,   209,   210,   183,   336,   337,   -16,   299,   300,
     301,   -16,   -16,   184,   433,   311,    41,   185,   377,   378,
     195,   196,   254,   293,   293,   111,   312,   227,  -140,  -140,
     231,   186,   200,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   325,   326,   111,   111,   111,   111,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   278,   278,   359,     9,   187,   188,   362,
     364,   376,   111,   368,   370,   290,   328,   382,   189,   329,
     330,   201,   277,   279,   280,   286,   405,   383,   190,   209,
     210,   197,   198,   103,   103,   262,   267,   425,   314,   203,
     209,   210,   365,   366,   218,   384,   327,   334,   331,    41,
     309,   335,   248,   220,   221,   222,   223,   224,   225,   338,
     226,   216,   339,   431,   341,   399,   400,   372,   374,   202,
     220,   221,   222,   223,   224,   225,   373,   226,   379,   385,
     387,   389,   278,   293,   293,   388,   390,   278,   278,   391,
     111,   111,   392,   393,   394,   234,   235,   236,   237,   238,
     239,   211,   212,   213,   214,   215,   395,   376,   376,   403,
     407,   396,   103,   397,   255,   413,   414,   103,   103,   416,
     422,    31,   423,   426,   435,   436,   204,   437,   357,   358,
     241,   242,   243,   244,   245,   246,   247,   438,   248,   444,
     449,   450,   453,   376,   128,   310,   124,   211,   212,   213,
     214,   215,   333,   434,     0,     0,   406,     0,   278,   278,
     429,     0,   430,     0,   200,     0,   227,     0,     0,   111,
       0,     0,     0,     0,   111,   111,   442,     0,   278,   278,
       0,   332,   418,   227,     0,     0,   419,   420,   103,   103,
       0,   236,   237,   238,   239,    41,   448,     0,     0,     0,
       0,   451,   211,   212,   213,   214,   215,   398,   103,   103,
       0,     0,   401,   402,  -141,  -141,     0,     0,   234,   235,
     236,   237,   238,   239,   241,   242,   243,   244,   245,   246,
     247,     0,   248,     0,     0,   234,   235,   236,   237,   238,
     239,    50,    51,    52,     9,   111,   111,    53,     0,     0,
       0,     0,    54,   241,   242,   243,   244,   245,   246,   247,
       0,   248,     0,     0,     0,   111,   111,     0,    55,    56,
     241,   242,   243,   244,   245,   246,   247,     0,   248,     0,
       0,     0,     0,   427,   428,     0,     0,     0,     0,     0,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,   439,   440,     0,     0,     0,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,   238,
     239,    80,    81,    82,    83,   243,   244,   245,   246,   247,
       0,   248,     0,     0,     0,     0,     0,     0,     0,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     0,     0,
     241,   242,   243,   244,   245,   246,   247,     0,   248,    31,
       0,     0,     0,     0,   113,    94,     0,     0,     0,     0,
       0,    95,     0,     0,     0,   114,   304,   305,    52,     9,
      10,     0,    53,   324,     0,     0,     0,    54,    11,    12,
      13,    14,    15,    16,    17,    18,     0,     0,    19,     0,
       0,    20,    21,   306,   307,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,     0,     0,
       0,     0,     0,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    24,     0,    80,    81,    82,    83,
      25,    26,     0,     0,    27,     0,    28,     0,     0,     0,
       0,     0,     0,     0,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    29,     0,    30,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,     0,     0,     0,    93,
      94,    33,     0,     0,     7,     8,    95,     9,    10,     0,
      96,     0,     0,     0,     0,    34,    11,    12,    13,    14,
      15,    16,    17,    18,     0,     0,    19,     0,     0,    20,
      21,    22,    23,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     0,     9,    10,     0,     0,     0,     0,     0,
       0,     0,    11,    12,    13,    14,    15,    16,    17,    18,
       0,     0,    19,     0,     0,    20,    21,    22,    23,   234,
     235,   236,   237,   238,   239,     0,     0,     0,     0,     0,
       0,     0,    24,     0,     0,     0,     0,     0,    25,    26,
       0,     0,    27,     0,    28,     0,     0,     0,     0,     0,
       0,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,    29,   248,    30,     0,     0,     0,     0,    24,     0,
       0,     0,    31,    32,    25,    26,     0,     0,    27,    33,
      28,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      42,     0,     0,    34,     0,     0,     0,    29,     0,    30,
       0,     0,     0,     0,     0,     0,     0,     0,    31,    32,
      50,    51,    52,     9,     0,    33,    53,     0,   132,   133,
     134,    54,     0,     0,     0,     0,     0,     0,     0,    34,
     135,   136,     0,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,     0,     0,     0,     0,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,     0,   159,   160,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
     161,   162,   163,   164,   165,   166,   167,   168,    31,     0,
       0,     0,     0,   113,    94,    50,    51,    52,     9,     0,
      95,    53,     0,     0,    96,     0,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,    56,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,     0,
       0,     0,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,     0,     0,    80,    81,    82,    83,     0,
      50,    51,    52,     9,     0,     0,    53,     0,     0,     0,
       0,    54,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,    31,     0,     0,     0,   284,    93,    94,
       0,     0,     0,     0,     0,    95,     0,     0,     0,   114,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,    50,    51,    52,     9,     0,
       0,    53,     0,     0,     0,     0,    54,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
       0,     0,    55,    56,     0,     0,     0,     0,    31,     0,
       0,     0,     0,    93,    94,     0,     0,     0,     0,     0,
      95,     0,     0,     0,    96,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,     0,
       0,     0,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,     0,     0,    80,    81,    82,    83,     0,
      50,    51,    52,     9,     0,     0,    53,     0,     0,     0,
       0,    54,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,    31,     0,     0,     0,     0,   113,    94,
       0,     0,     0,     0,     0,    95,     0,     0,     0,   114,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,    50,    51,    52,     9,     0,
       0,    53,     0,     0,     0,     0,    54,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
       0,     0,    55,    56,     0,     0,     0,     0,    31,     0,
       0,     0,     0,    93,    94,     0,     0,     0,     0,     0,
      95,     0,     0,     0,   114,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,     0,
       0,     0,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,     0,     0,    80,    81,    82,    83,     0,
     281,    51,    52,     0,     0,     0,   282,     0,     0,     0,
       0,   283,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,    31,     0,     0,     0,     0,   291,    94,
       0,     0,     0,     0,     0,    95,     0,     0,     0,   114,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,    50,    51,    52,     0,     0,
       0,    53,     0,     0,     0,     0,    54,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
       0,     0,    55,    56,     0,     0,     0,     0,     0,     0,
       0,     0,   284,   285,    94,     0,     0,     0,     0,     0,
      95,     0,     0,     0,    96,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,     0,
       0,     0,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,     0,     0,    80,    81,    82,    83,     0,
      50,    51,    52,     0,     0,     0,    53,     0,     0,     0,
       0,    54,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   113,    94,
       0,     0,     0,     0,     0,    95,   447,     0,     0,    96,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,    50,    51,    52,     0,     0,
       0,    53,     0,     0,     0,     0,    54,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
       0,     0,    55,    56,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   113,    94,     0,     0,     0,     0,     0,
      95,     0,     0,     0,    96,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,     0,
       0,     0,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,     0,     0,    80,    81,    82,    83,     0,
      50,    51,    52,     0,     0,     0,    53,     0,     0,     0,
       0,    54,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    93,    94,
       0,     0,     0,     0,     0,    95,     0,     0,     0,    96,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,     0,     0,     0,     0,     0,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,     0,     0,
      80,    81,    82,    83,     0,     0,    50,    51,    52,     0,
       0,     0,    53,     0,     0,     0,     0,    54,    84,    85,
      86,    87,    88,    89,    90,    91,    92,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   291,    94,     0,     0,     0,     0,     0,
      95,  -205,     0,     0,    96,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,     0,     0,
       0,     0,     0,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,     0,     0,    80,    81,    82,    83,
       0,     0,    50,    51,    52,     0,     0,     0,    53,     0,
       0,     0,     0,    54,    84,    85,    86,    87,    88,    89,
      90,    91,    92,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   113,
      94,     0,     0,     0,     0,     0,    95,  -204,     0,     0,
      96,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,     0,     0,     0,     0,     0,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
       0,     0,    80,    81,    82,    83,     0,    50,    51,    52,
       0,     0,     0,    53,     0,     0,     0,     0,    54,     0,
      84,    85,    86,    87,    88,    89,    90,    91,    92,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   113,    94,     0,     0,     0,
       0,     0,    95,     0,     0,     0,    96,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,     0,
       0,     0,     0,     0,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,     0,     0,    80,    81,    82,
      83,    55,    56,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    84,    85,    86,    87,    88,
      89,    90,    91,    92,     0,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    55,    56,     0,     0,
     113,    94,     0,     0,     0,     0,     0,    95,     0,     0,
       0,    96,     0,     0,    80,    81,    82,    83,     0,     0,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,     0,    84,    85,    86,    87,    88,    89,    90,    91,
      92,     0,     0,     0,     0,   232,     0,     0,   233,    80,
      81,    82,    83,     0,     0,   232,   381,     0,   233,     0,
       0,     0,     0,     0,     0,     0,     0,    84,    85,    86,
      87,    88,    89,    90,    91,    92,   259,   260,   236,   237,
     238,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   284,   234,   235,   236,   237,   238,   239,     0,     0,
       0,     0,   259,   260,   236,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,     0,   248,
       0,     0,     0,     0,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,     0,   248,   240,   241,   242,   243,
     244,   245,   246,   247,     0,   248,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,     0,     0,
       0,     0,     0,     0,     0,    80,    81,    82,    83,     0,
       0,     0,     0,     0,     0,     0,    80,    81,    82,    83,
       0,     0,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,     0,     0,    84,    85,    86,    87,    88,    89,
      90,    91,    92,     0,     0,     0,     0,   216,   259,   260,
     236,   237,   238,   239,     0,     0,     0,   234,   235,   236,
     237,   238,   239,     0,     0,     0,   234,     0,   236,   237,
     238,   239,     0,     0,     0,   259,     0,   236,   237,   238,
     239,     0,     0,   241,   242,   243,   244,   245,   246,   247,
       0,   248,   241,   242,   243,   244,   245,   246,   247,     0,
     248,   241,   242,   243,   244,   245,   246,   247,     0,   248,
     241,   242,   243,   244,   245,   246,   247,     0,   248
};

static const yytype_int16 yycheck[] =
{
      26,   128,   106,    52,    30,    52,    32,     4,   108,     4,
     129,   114,     0,     3,    40,     6,     0,   140,   141,     9,
       0,    97,   132,   133,    14,   139,   145,     3,     0,     6,
      26,    26,   139,     9,   140,   145,    32,    32,    14,     3,
       4,    28,   281,   141,   171,    40,   285,    35,   124,    93,
       4,    30,    31,     4,   104,    43,   126,    44,    48,   132,
     133,    94,   134,   135,   136,     6,   138,    93,    82,    95,
      96,    97,   145,    14,    15,    16,    17,    18,    19,    20,
     140,    22,     8,   132,   133,   132,   133,    26,   114,   132,
     133,    82,   126,    32,    95,    96,   146,    93,   124,    97,
      98,    97,    97,   147,   131,   132,   133,    94,   126,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   101,   102,   150,   151,   152,   122,   124,   124,
     230,   130,   129,   128,   126,   161,   162,   127,   377,   378,
     166,   167,   125,   126,   121,   171,   140,   143,   144,   145,
     146,   127,   255,   256,    93,   139,   259,   260,    97,   143,
     144,   131,   132,   133,   126,   209,   210,   139,   163,   164,
     165,   143,   144,   126,   413,   171,   171,   126,   282,   283,
      30,    31,    37,   209,   210,   124,   173,   128,   132,   133,
     290,   126,    93,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   143,   144,   145,   146,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,     6,   126,   126,   255,
     256,   281,   171,   259,   260,   285,   129,   287,   126,   132,
     133,   285,   143,   144,   145,   146,   129,   291,   126,   132,
     133,   101,   102,   249,   250,     6,     4,   129,     8,   285,
     132,   133,   257,   258,     6,   291,   145,   130,   145,   264,
     171,    10,   138,    15,    16,    17,    18,    19,    20,     3,
      22,   125,   144,   410,    49,   329,   330,   144,     8,   285,
      15,    16,    17,    18,    19,    20,   143,    22,     3,     8,
     145,   129,   328,   329,   330,   145,   145,   333,   334,   145,
     249,   250,   145,   129,   129,    95,    96,    97,    98,    99,
     100,    65,    66,    67,    68,    69,   145,   377,   378,    50,
       8,   145,   328,   145,    95,   129,     3,   333,   334,   111,
     145,   121,   145,    51,     6,   145,   285,   145,   249,   250,
     130,   131,   132,   133,   134,   135,   136,   145,   138,   145,
      92,   145,     8,   413,    35,   171,    32,    65,    66,    67,
      68,    69,   203,   415,    -1,    -1,    33,    -1,   404,   405,
     406,    -1,   408,    -1,   285,    -1,   128,    -1,    -1,   328,
      -1,    -1,    -1,    -1,   333,   334,    36,    -1,   424,   425,
      -1,   145,   389,   128,    -1,    -1,   393,   394,   404,   405,
      -1,    97,    98,    99,   100,   410,   442,    -1,    -1,    -1,
      -1,   447,    65,    66,    67,    68,    69,   328,   424,   425,
      -1,    -1,   333,   334,   132,   133,    -1,    -1,    95,    96,
      97,    98,    99,   100,   130,   131,   132,   133,   134,   135,
     136,    -1,   138,    -1,    -1,    95,    96,    97,    98,    99,
     100,     3,     4,     5,     6,   404,   405,     9,    -1,    -1,
      -1,    -1,    14,   130,   131,   132,   133,   134,   135,   136,
      -1,   138,    -1,    -1,    -1,   424,   425,    -1,    30,    31,
     130,   131,   132,   133,   134,   135,   136,    -1,   138,    -1,
      -1,    -1,    -1,   404,   405,    -1,    -1,    -1,    -1,    -1,
      -1,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,   424,   425,    -1,    -1,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    99,
     100,    83,    84,    85,    86,   132,   133,   134,   135,   136,
      -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   101,
     102,   103,   104,   105,   106,   107,   108,   109,    -1,    -1,
     130,   131,   132,   133,   134,   135,   136,    -1,   138,   121,
      -1,    -1,    -1,    -1,   126,   127,    -1,    -1,    -1,    -1,
      -1,   133,    -1,    -1,    -1,   137,     3,     4,     5,     6,
       7,    -1,     9,   145,    -1,    -1,    -1,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    -1,    -1,    25,    -1,
      -1,    28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    83,    84,    85,    86,
      87,    88,    -1,    -1,    91,    -1,    93,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   121,   122,    -1,    -1,    -1,   126,
     127,   128,    -1,    -1,     3,     4,   133,     6,     7,    -1,
     137,    -1,    -1,    -1,    -1,   142,    15,    16,    17,    18,
      19,    20,    21,    22,    -1,    -1,    25,    -1,    -1,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,    -1,     6,     7,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    -1,    25,    -1,    -1,    28,    29,    30,    31,    95,
      96,    97,    98,    99,   100,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81,    -1,    -1,    -1,    -1,    -1,    87,    88,
      -1,    -1,    91,    -1,    93,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   129,   130,   131,   132,   133,   134,   135,
     136,   110,   138,   112,    -1,    -1,    -1,    -1,    81,    -1,
      -1,    -1,   121,   122,    87,    88,    -1,    -1,    91,   128,
      93,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,    -1,    -1,   142,    -1,    -1,    -1,   110,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
       3,     4,     5,     6,    -1,   128,     9,    -1,    11,    12,
      13,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      23,    24,    -1,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,    -1,    89,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,    -1,
      -1,    -1,    -1,   126,   127,     3,     4,     5,     6,    -1,
     133,     9,    -1,    -1,   137,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,    83,    84,    85,    86,    -1,
       3,     4,     5,     6,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    14,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,   121,    -1,    -1,    -1,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,   137,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    14,    -1,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,   121,    -1,
      -1,    -1,    -1,   126,   127,    -1,    -1,    -1,    -1,    -1,
     133,    -1,    -1,    -1,   137,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,    83,    84,    85,    86,    -1,
       3,     4,     5,     6,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    14,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,   121,    -1,    -1,    -1,    -1,   126,   127,
      -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,   137,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    14,    -1,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,   121,    -1,
      -1,    -1,    -1,   126,   127,    -1,    -1,    -1,    -1,    -1,
     133,    -1,    -1,    -1,   137,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,    83,    84,    85,    86,    -1,
       3,     4,     5,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    14,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,   121,    -1,    -1,    -1,    -1,   126,   127,
      -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,   137,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,     3,     4,     5,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    14,    -1,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,
     133,    -1,    -1,    -1,   137,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,    83,    84,    85,    86,    -1,
       3,     4,     5,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    14,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,   127,
      -1,    -1,    -1,    -1,    -1,   133,   134,    -1,    -1,   137,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,     3,     4,     5,    -1,    -1,
      -1,     9,    -1,    -1,    -1,    -1,    14,    -1,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   126,   127,    -1,    -1,    -1,    -1,    -1,
     133,    -1,    -1,    -1,   137,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    -1,    -1,    83,    84,    85,    86,    -1,
       3,     4,     5,    -1,    -1,    -1,     9,    -1,    -1,    -1,
      -1,    14,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,   127,
      -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,   137,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    -1,
      83,    84,    85,    86,    -1,    -1,     3,     4,     5,    -1,
      -1,    -1,     9,    -1,    -1,    -1,    -1,    14,   101,   102,
     103,   104,   105,   106,   107,   108,   109,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   126,   127,    -1,    -1,    -1,    -1,    -1,
     133,    48,    -1,    -1,   137,    -1,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    -1,    -1,    83,    84,    85,    86,
      -1,    -1,     3,     4,     5,    -1,    -1,    -1,     9,    -1,
      -1,    -1,    -1,    14,   101,   102,   103,   104,   105,   106,
     107,   108,   109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,
     127,    -1,    -1,    -1,    -1,    -1,   133,    48,    -1,    -1,
     137,    -1,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      -1,    -1,    83,    84,    85,    86,    -1,     3,     4,     5,
      -1,    -1,    -1,     9,    -1,    -1,    -1,    -1,    14,    -1,
     101,   102,   103,   104,   105,   106,   107,   108,   109,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   126,   127,    -1,    -1,    -1,
      -1,    -1,   133,    -1,    -1,    -1,   137,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    -1,    -1,    83,    84,    85,
      86,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   101,   102,   103,   104,   105,
     106,   107,   108,   109,    -1,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    30,    31,    -1,    -1,
     126,   127,    -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,
      -1,   137,    -1,    -1,    83,    84,    85,    86,    -1,    -1,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    -1,   101,   102,   103,   104,   105,   106,   107,   108,
     109,    -1,    -1,    -1,    -1,    48,    -1,    -1,    51,    83,
      84,    85,    86,    -1,    -1,    48,   125,    -1,    51,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   101,   102,   103,
     104,   105,   106,   107,   108,   109,    95,    96,    97,    98,
      99,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   125,    95,    96,    97,    98,    99,   100,    -1,    -1,
      -1,    -1,    95,    96,    97,    98,    99,   100,    -1,    -1,
     129,   130,   131,   132,   133,   134,   135,   136,    -1,   138,
      -1,    -1,    -1,    -1,    -1,    -1,   129,   130,   131,   132,
     133,   134,   135,   136,    -1,   138,   129,   130,   131,   132,
     133,   134,   135,   136,    -1,   138,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,    86,
      -1,    -1,    -1,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,   101,   102,   103,   104,   105,   106,
     107,   108,   109,    -1,    -1,    -1,    -1,   125,    95,    96,
      97,    98,    99,   100,    -1,    -1,    -1,    95,    96,    97,
      98,    99,   100,    -1,    -1,    -1,    95,    -1,    97,    98,
      99,   100,    -1,    -1,    -1,    95,    -1,    97,    98,    99,
     100,    -1,    -1,   130,   131,   132,   133,   134,   135,   136,
      -1,   138,   130,   131,   132,   133,   134,   135,   136,    -1,
     138,   130,   131,   132,   133,   134,   135,   136,    -1,   138,
     130,   131,   132,   133,   134,   135,   136,    -1,   138
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   139,   147,   148,   150,   151,     0,     3,     4,     6,
       7,    15,    16,    17,    18,    19,    20,    21,    22,    25,
      28,    29,    30,    31,    81,    87,    88,    91,    93,   110,
     112,   121,   122,   128,   142,   149,   152,   161,   164,   170,
     174,   176,   139,   141,   140,   141,   140,     6,    82,   153,
       3,     4,     5,     9,    14,    30,    31,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      83,    84,    85,    86,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   126,   127,   133,   137,   162,   163,   176,
     178,   179,   181,   182,   183,   184,   185,   186,   191,   192,
     193,   194,     4,   126,   137,   167,   168,   176,   182,   192,
     194,     4,   192,   126,   162,   175,   171,   150,   151,     4,
     129,    94,    11,    12,    13,    23,    24,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    89,
      90,   113,   114,   115,   116,   117,   118,   119,   120,   176,
     192,   150,   167,   140,     3,     4,   160,    82,   156,     8,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,    30,    31,   101,   102,    30,    31,   101,   102,   167,
     178,   181,   182,   192,   194,   167,   192,   192,   163,   132,
     133,    65,    66,    67,    68,    69,   125,   191,     6,    14,
      15,    16,    17,    18,    19,    20,    22,   128,   187,   187,
      48,   183,    48,    51,    95,    96,    97,    98,    99,   100,
     129,   130,   131,   132,   133,   134,   135,   136,   138,   180,
     130,   140,   168,   192,    37,    95,    96,    97,    98,    95,
      96,   176,     6,   148,   150,   148,   170,     4,   159,   192,
     192,   192,   192,   192,   192,   192,   192,   178,   192,   178,
     178,     3,     9,    14,   125,   126,   178,   188,   189,   190,
     191,   126,   181,   192,   192,   192,   192,   192,   192,   176,
     176,   176,   192,   192,     3,     4,    30,    31,   170,   178,
     179,   182,   167,   154,     8,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   145,   167,   167,   145,   129,   132,
     133,   145,   145,   180,   130,    10,   181,   181,     3,   144,
     183,    49,   192,   192,   192,   192,   192,   192,   192,   192,
     192,   192,   192,   192,   192,   192,   192,   178,   178,   192,
     165,   168,   192,   168,   192,   176,   176,   168,   192,   168,
     192,   177,   144,   143,     8,   189,   191,   187,   187,     3,
     189,   125,   191,   181,   192,     8,   157,   145,   145,   129,
     145,   145,   145,   129,   129,   145,   145,   145,   178,   181,
     181,   178,   178,    50,    52,   129,    33,     8,   129,   145,
     172,   189,   189,   129,     3,   155,   111,   166,   167,   167,
     167,   145,   145,   145,    52,   129,    51,   178,   178,   192,
     192,   170,   173,   189,   166,     6,   145,   145,   145,   178,
     178,   131,    36,   169,   145,   145,   131,   134,   192,    92,
     145,   192,   158,     8
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 279 "pic.y"
    {
		  if (olist.head)
		    print_picture(olist.head);
		}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 288 "pic.y"
    { (yyval.pl) = (yyvsp[(2) - (3)].pl); }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 293 "pic.y"
    { (yyval.pl) = (yyvsp[(1) - (1)].pl); }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 295 "pic.y"
    { (yyval.pl) = (yyvsp[(1) - (3)].pl); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 310 "pic.y"
    {
		  a_delete graphname;
		  graphname = new char[strlen((yyvsp[(3) - (3)].str)) + 1];
		  strcpy(graphname, (yyvsp[(3) - (3)].str));
		  a_delete (yyvsp[(3) - (3)].str);
		}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 318 "pic.y"
    {
		  define_variable((yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].x));
		  a_delete (yyvsp[(1) - (3)].str);
		}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 323 "pic.y"
    {
		  place *p = lookup_label((yyvsp[(1) - (4)].str));
		  if (!p) {
		    lex_error("variable `%1' not defined", (yyvsp[(1) - (4)].str));
		    YYABORT;
		  }
		  p->obj = 0;
		  p->x = (yyvsp[(4) - (4)].x);
		  p->y = 0.0;
		  a_delete (yyvsp[(1) - (4)].str);
		}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 335 "pic.y"
    { current_direction = UP_DIRECTION; }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 337 "pic.y"
    { current_direction = DOWN_DIRECTION; }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 339 "pic.y"
    { current_direction = LEFT_DIRECTION; }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 341 "pic.y"
    { current_direction = RIGHT_DIRECTION; }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 343 "pic.y"
    {
		  olist.append(make_command_object((yyvsp[(1) - (1)].lstr).str, (yyvsp[(1) - (1)].lstr).filename,
						   (yyvsp[(1) - (1)].lstr).lineno));
		}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 348 "pic.y"
    {
		  olist.append(make_command_object((yyvsp[(2) - (2)].lstr).str, (yyvsp[(2) - (2)].lstr).filename,
						   (yyvsp[(2) - (2)].lstr).lineno));
		}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 353 "pic.y"
    {
		  fprintf(stderr, "%s\n", (yyvsp[(2) - (2)].lstr).str);
		  a_delete (yyvsp[(2) - (2)].lstr).str;
		  fflush(stderr);
		}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 359 "pic.y"
    { delim_flag = 1; }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 361 "pic.y"
    {
		  delim_flag = 0;
		  if (safer_flag)
		    lex_error("unsafe to run command `%1'", (yyvsp[(3) - (3)].str));
		  else
		    system((yyvsp[(3) - (3)].str));
		  a_delete (yyvsp[(3) - (3)].str);
		}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 370 "pic.y"
    {
		  if (yychar < 0)
		    do_lookahead();
		  do_copy((yyvsp[(2) - (2)].lstr).str);
		  // do not delete the filename
		}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 377 "pic.y"
    { delim_flag = 2; }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 379 "pic.y"
    { delim_flag = 0; }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 381 "pic.y"
    {
		  if (yychar < 0)
		    do_lookahead();
		  copy_file_thru((yyvsp[(2) - (7)].lstr).str, (yyvsp[(5) - (7)].str), (yyvsp[(7) - (7)].str));
		  // do not delete the filename
		  a_delete (yyvsp[(5) - (7)].str);
		  a_delete (yyvsp[(7) - (7)].str);
		}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 390 "pic.y"
    { delim_flag = 2; }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 392 "pic.y"
    { delim_flag = 0; }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 394 "pic.y"
    {
		  if (yychar < 0)
		    do_lookahead();
		  copy_rest_thru((yyvsp[(4) - (6)].str), (yyvsp[(6) - (6)].str));
		  a_delete (yyvsp[(4) - (6)].str);
		  a_delete (yyvsp[(6) - (6)].str);
		}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 402 "pic.y"
    { delim_flag = 1; }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 404 "pic.y"
    {
		  delim_flag = 0;
		  if (yychar < 0)
		    do_lookahead();
		  do_for((yyvsp[(2) - (10)].str), (yyvsp[(4) - (10)].x), (yyvsp[(6) - (10)].x), (yyvsp[(7) - (10)].by).is_multiplicative, (yyvsp[(7) - (10)].by).val, (yyvsp[(10) - (10)].str)); 
		}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 411 "pic.y"
    {
		  if (yychar < 0)
		    do_lookahead();
		  if ((yyvsp[(1) - (1)].if_data).x != 0.0)
		    push_body((yyvsp[(1) - (1)].if_data).body);
		  a_delete (yyvsp[(1) - (1)].if_data).body;
		}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 419 "pic.y"
    { delim_flag = 1; }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 421 "pic.y"
    {
		  delim_flag = 0;
		  if (yychar < 0)
		    do_lookahead();
		  if ((yyvsp[(1) - (4)].if_data).x != 0.0)
		    push_body((yyvsp[(1) - (4)].if_data).body);
		  else
		    push_body((yyvsp[(4) - (4)].str));
		  a_delete (yyvsp[(1) - (4)].if_data).body;
		  a_delete (yyvsp[(4) - (4)].str);
		}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 434 "pic.y"
    { define_variable("scale", 1.0); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 444 "pic.y"
    {
		  reset((yyvsp[(2) - (2)].str));
		  a_delete (yyvsp[(2) - (2)].str);
		}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 449 "pic.y"
    {
		  reset((yyvsp[(2) - (2)].str));
		  a_delete (yyvsp[(2) - (2)].str);
		}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 454 "pic.y"
    {
		  reset((yyvsp[(3) - (3)].str));
		  a_delete (yyvsp[(3) - (3)].str);
		}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 462 "pic.y"
    { (yyval.lstr) = (yyvsp[(1) - (1)].lstr); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 464 "pic.y"
    {
		  (yyval.lstr).str = new char[strlen((yyvsp[(1) - (2)].lstr).str) + strlen((yyvsp[(2) - (2)].lstr).str) + 1];
		  strcpy((yyval.lstr).str, (yyvsp[(1) - (2)].lstr).str);
		  strcat((yyval.lstr).str, (yyvsp[(2) - (2)].lstr).str);
		  a_delete (yyvsp[(1) - (2)].lstr).str;
		  a_delete (yyvsp[(2) - (2)].lstr).str;
		  if ((yyvsp[(1) - (2)].lstr).filename) {
		    (yyval.lstr).filename = (yyvsp[(1) - (2)].lstr).filename;
		    (yyval.lstr).lineno = (yyvsp[(1) - (2)].lstr).lineno;
		  }
		  else if ((yyvsp[(2) - (2)].lstr).filename) {
		    (yyval.lstr).filename = (yyvsp[(2) - (2)].lstr).filename;
		    (yyval.lstr).lineno = (yyvsp[(2) - (2)].lstr).lineno;
		  }
		}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 483 "pic.y"
    {
		  (yyval.lstr).str = new char[GDIGITS + 1];
		  sprintf((yyval.lstr).str, "%g", (yyvsp[(1) - (1)].x));
		  (yyval.lstr).filename = 0;
		  (yyval.lstr).lineno = 0;
		}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 490 "pic.y"
    { (yyval.lstr) = (yyvsp[(1) - (1)].lstr); }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 492 "pic.y"
    {
		  (yyval.lstr).str = new char[GDIGITS + 2 + GDIGITS + 1];
		  sprintf((yyval.lstr).str, "%g, %g", (yyvsp[(1) - (1)].pair).x, (yyvsp[(1) - (1)].pair).y);
		  (yyval.lstr).filename = 0;
		  (yyval.lstr).lineno = 0;
		}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 502 "pic.y"
    { delim_flag = 1; }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 504 "pic.y"
    {
		  delim_flag = 0;
		  (yyval.if_data).x = (yyvsp[(2) - (5)].x);
		  (yyval.if_data).body = (yyvsp[(5) - (5)].str);
		}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 513 "pic.y"
    { (yyval.str) = 0; }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 515 "pic.y"
    { (yyval.str) = (yyvsp[(2) - (2)].lstr).str; }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 520 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (1)].x); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 522 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (1)].x); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 527 "pic.y"
    {
		  (yyval.x) = strcmp((yyvsp[(1) - (3)].lstr).str, (yyvsp[(3) - (3)].lstr).str) == 0;
		  a_delete (yyvsp[(1) - (3)].lstr).str;
		  a_delete (yyvsp[(3) - (3)].lstr).str;
		}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 533 "pic.y"
    {
		  (yyval.x) = strcmp((yyvsp[(1) - (3)].lstr).str, (yyvsp[(3) - (3)].lstr).str) != 0;
		  a_delete (yyvsp[(1) - (3)].lstr).str;
		  a_delete (yyvsp[(3) - (3)].lstr).str;
		}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 539 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 && (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 541 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 && (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 543 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 && (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 545 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 || (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 547 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 || (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 549 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 || (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 551 "pic.y"
    { (yyval.x) = ((yyvsp[(2) - (2)].x) == 0.0); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 557 "pic.y"
    {
		  (yyval.by).val = 1.0;
		  (yyval.by).is_multiplicative = 0;
		}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 562 "pic.y"
    {
		  (yyval.by).val = (yyvsp[(2) - (2)].x);
		  (yyval.by).is_multiplicative = 0;
		}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 567 "pic.y"
    {
		  (yyval.by).val = (yyvsp[(3) - (3)].x);
		  (yyval.by).is_multiplicative = 1;
		}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 575 "pic.y"
    {
		  (yyval.pl).obj = (yyvsp[(1) - (1)].spec)->make_object(&current_position,
					   &current_direction);
		  if ((yyval.pl).obj == 0)
		    YYABORT;
		  delete (yyvsp[(1) - (1)].spec);
		  if ((yyval.pl).obj)
		    olist.append((yyval.pl).obj);
		  else {
		    (yyval.pl).x = current_position.x;
		    (yyval.pl).y = current_position.y;
		  }
		}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 589 "pic.y"
    {
		  (yyval.pl) = (yyvsp[(4) - (4)].pl);
		  define_label((yyvsp[(1) - (4)].str), & (yyval.pl));
		  a_delete (yyvsp[(1) - (4)].str);
		}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 595 "pic.y"
    {
		  (yyval.pl).obj = 0;
		  (yyval.pl).x = (yyvsp[(4) - (4)].pair).x;
		  (yyval.pl).y = (yyvsp[(4) - (4)].pair).y;
		  define_label((yyvsp[(1) - (4)].str), & (yyval.pl));
		  a_delete (yyvsp[(1) - (4)].str);
		}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 603 "pic.y"
    {
		  (yyval.pl) = (yyvsp[(4) - (4)].pl);
		  define_label((yyvsp[(1) - (4)].str), & (yyval.pl));
		  a_delete (yyvsp[(1) - (4)].str);
		}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 609 "pic.y"
    {
		  (yyval.state).x = current_position.x;
		  (yyval.state).y = current_position.y;
		  (yyval.state).dir = current_direction;
		}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 615 "pic.y"
    {
		  current_position.x = (yyvsp[(2) - (4)].state).x;
		  current_position.y = (yyvsp[(2) - (4)].state).y;
		  current_direction = (yyvsp[(2) - (4)].state).dir;
		}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 621 "pic.y"
    {
		  (yyval.pl) = (yyvsp[(3) - (6)].pl);
		}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 625 "pic.y"
    {
		  (yyval.pl).obj = 0;
		  (yyval.pl).x = current_position.x;
		  (yyval.pl).y = current_position.y;
		}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 634 "pic.y"
    {}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 636 "pic.y"
    {}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 641 "pic.y"
    { (yyval.spec) = new object_spec(BOX_OBJECT); }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 643 "pic.y"
    { (yyval.spec) = new object_spec(CIRCLE_OBJECT); }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 645 "pic.y"
    { (yyval.spec) = new object_spec(ELLIPSE_OBJECT); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 647 "pic.y"
    {
		  (yyval.spec) = new object_spec(ARC_OBJECT);
		  (yyval.spec)->dir = current_direction;
		}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 652 "pic.y"
    {
		  (yyval.spec) = new object_spec(LINE_OBJECT);
		  lookup_variable("lineht", & (yyval.spec)->segment_height);
		  lookup_variable("linewid", & (yyval.spec)->segment_width);
		  (yyval.spec)->dir = current_direction;
		}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 659 "pic.y"
    {
		  (yyval.spec) = new object_spec(ARROW_OBJECT);
		  lookup_variable("lineht", & (yyval.spec)->segment_height);
		  lookup_variable("linewid", & (yyval.spec)->segment_width);
		  (yyval.spec)->dir = current_direction;
		}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 666 "pic.y"
    {
		  (yyval.spec) = new object_spec(MOVE_OBJECT);
		  lookup_variable("moveht", & (yyval.spec)->segment_height);
		  lookup_variable("movewid", & (yyval.spec)->segment_width);
		  (yyval.spec)->dir = current_direction;
		}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 673 "pic.y"
    {
		  (yyval.spec) = new object_spec(SPLINE_OBJECT);
		  lookup_variable("lineht", & (yyval.spec)->segment_height);
		  lookup_variable("linewid", & (yyval.spec)->segment_width);
		  (yyval.spec)->dir = current_direction;
		}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 680 "pic.y"
    {
		  (yyval.spec) = new object_spec(TEXT_OBJECT);
		  (yyval.spec)->text = new text_item((yyvsp[(1) - (1)].lstr).str, (yyvsp[(1) - (1)].lstr).filename, (yyvsp[(1) - (1)].lstr).lineno);
		}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 685 "pic.y"
    {
		  (yyval.spec) = new object_spec(TEXT_OBJECT);
		  (yyval.spec)->text = new text_item(format_number(0, (yyvsp[(2) - (2)].x)), 0, -1);
		}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 690 "pic.y"
    {
		  (yyval.spec) = new object_spec(TEXT_OBJECT);
		  (yyval.spec)->text = new text_item(format_number((yyvsp[(3) - (3)].lstr).str, (yyvsp[(2) - (3)].x)),
					   (yyvsp[(3) - (3)].lstr).filename, (yyvsp[(3) - (3)].lstr).lineno);
		  a_delete (yyvsp[(3) - (3)].lstr).str;
		}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 697 "pic.y"
    {
		  saved_state *p = new saved_state;
		  (yyval.pstate) = p;
		  p->x = current_position.x;
		  p->y = current_position.y;
		  p->dir = current_direction;
		  p->tbl = current_table;
		  p->prev = current_saved_state;
		  current_position.x = 0.0;
		  current_position.y = 0.0;
		  current_table = new PTABLE(place);
		  current_saved_state = p;
		  olist.append(make_mark_object());
		}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 712 "pic.y"
    {
		  current_position.x = (yyvsp[(2) - (4)].pstate)->x;
		  current_position.y = (yyvsp[(2) - (4)].pstate)->y;
		  current_direction = (yyvsp[(2) - (4)].pstate)->dir;
		  (yyval.spec) = new object_spec(BLOCK_OBJECT);
		  olist.wrap_up_block(& (yyval.spec)->oblist);
		  (yyval.spec)->tbl = current_table;
		  current_table = (yyvsp[(2) - (4)].pstate)->tbl;
		  current_saved_state = (yyvsp[(2) - (4)].pstate)->prev;
		  delete (yyvsp[(2) - (4)].pstate);
		}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 724 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->height = (yyvsp[(3) - (3)].x);
		  (yyval.spec)->flags |= HAS_HEIGHT;
		}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 730 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->radius = (yyvsp[(3) - (3)].x);
		  (yyval.spec)->flags |= HAS_RADIUS;
		}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 736 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->width = (yyvsp[(3) - (3)].x);
		  (yyval.spec)->flags |= HAS_WIDTH;
		}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 742 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->radius = (yyvsp[(3) - (3)].x)/2.0;
		  (yyval.spec)->flags |= HAS_RADIUS;
		}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 748 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  switch ((yyval.spec)->dir) {
		  case UP_DIRECTION:
		    (yyval.spec)->segment_pos.y += (yyvsp[(2) - (2)].x);
		    break;
		  case DOWN_DIRECTION:
		    (yyval.spec)->segment_pos.y -= (yyvsp[(2) - (2)].x);
		    break;
		  case RIGHT_DIRECTION:
		    (yyval.spec)->segment_pos.x += (yyvsp[(2) - (2)].x);
		    break;
		  case LEFT_DIRECTION:
		    (yyval.spec)->segment_pos.x -= (yyvsp[(2) - (2)].x);
		    break;
		  }
		}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 767 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->dir = UP_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.y += (yyval.spec)->segment_height;
		}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 774 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->dir = UP_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.y += (yyvsp[(3) - (3)].x);
		}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 781 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->dir = DOWN_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.y -= (yyval.spec)->segment_height;
		}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 788 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->dir = DOWN_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.y -= (yyvsp[(3) - (3)].x);
		}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 795 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->dir = RIGHT_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x += (yyval.spec)->segment_width;
		}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 802 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->dir = RIGHT_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x += (yyvsp[(3) - (3)].x);
		}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 809 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->dir = LEFT_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x -= (yyval.spec)->segment_width;
		}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 816 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->dir = LEFT_DIRECTION;
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x -= (yyvsp[(3) - (3)].x);
		}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 823 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_FROM;
		  (yyval.spec)->from.x = (yyvsp[(3) - (3)].pair).x;
		  (yyval.spec)->from.y = (yyvsp[(3) - (3)].pair).y;
		}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 830 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  if ((yyval.spec)->flags & HAS_SEGMENT)
		    (yyval.spec)->segment_list = new segment((yyval.spec)->segment_pos,
						   (yyval.spec)->segment_is_absolute,
						   (yyval.spec)->segment_list);
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x = (yyvsp[(3) - (3)].pair).x;
		  (yyval.spec)->segment_pos.y = (yyvsp[(3) - (3)].pair).y;
		  (yyval.spec)->segment_is_absolute = 1;
		  (yyval.spec)->flags |= HAS_TO;
		  (yyval.spec)->to.x = (yyvsp[(3) - (3)].pair).x;
		  (yyval.spec)->to.y = (yyvsp[(3) - (3)].pair).y;
		}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 845 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_AT;
		  (yyval.spec)->at.x = (yyvsp[(3) - (3)].pair).x;
		  (yyval.spec)->at.y = (yyvsp[(3) - (3)].pair).y;
		  if ((yyval.spec)->type != ARC_OBJECT) {
		    (yyval.spec)->flags |= HAS_FROM;
		    (yyval.spec)->from.x = (yyvsp[(3) - (3)].pair).x;
		    (yyval.spec)->from.y = (yyvsp[(3) - (3)].pair).y;
		  }
		}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 857 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_WITH;
		  (yyval.spec)->with = (yyvsp[(3) - (3)].pth);
		}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 863 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_WITH;
		  position pos;
		  pos.x = (yyvsp[(3) - (3)].pair).x;
		  pos.y = (yyvsp[(3) - (3)].pair).y;
		  (yyval.spec)->with = new path(pos);
		}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 872 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x += (yyvsp[(3) - (3)].pair).x;
		  (yyval.spec)->segment_pos.y += (yyvsp[(3) - (3)].pair).y;
		}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 879 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  if (!((yyval.spec)->flags & HAS_SEGMENT))
		    switch ((yyval.spec)->dir) {
		    case UP_DIRECTION:
		      (yyval.spec)->segment_pos.y += (yyval.spec)->segment_width;
		      break;
		    case DOWN_DIRECTION:
		      (yyval.spec)->segment_pos.y -= (yyval.spec)->segment_width;
		      break;
		    case RIGHT_DIRECTION:
		      (yyval.spec)->segment_pos.x += (yyval.spec)->segment_width;
		      break;
		    case LEFT_DIRECTION:
		      (yyval.spec)->segment_pos.x -= (yyval.spec)->segment_width;
		      break;
		    }
		  (yyval.spec)->segment_list = new segment((yyval.spec)->segment_pos,
						 (yyval.spec)->segment_is_absolute,
						 (yyval.spec)->segment_list);
		  (yyval.spec)->flags &= ~HAS_SEGMENT;
		  (yyval.spec)->segment_pos.x = (yyval.spec)->segment_pos.y = 0.0;
		  (yyval.spec)->segment_is_absolute = 0;
		}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 904 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);	// nothing
		}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 908 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_DOTTED;
		  lookup_variable("dashwid", & (yyval.spec)->dash_width);
		}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 914 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_DOTTED;
		  (yyval.spec)->dash_width = (yyvsp[(3) - (3)].x);
		}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 920 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_DASHED;
		  lookup_variable("dashwid", & (yyval.spec)->dash_width);
		}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 926 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_DASHED;
		  (yyval.spec)->dash_width = (yyvsp[(3) - (3)].x);
		}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 932 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_DEFAULT_FILLED;
		}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 937 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_FILLED;
		  (yyval.spec)->fill = (yyvsp[(3) - (3)].x);
		}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 943 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_XSLANTED;
		  (yyval.spec)->xslanted = (yyvsp[(3) - (3)].x);
		}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 949 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_YSLANTED;
		  (yyval.spec)->yslanted = (yyvsp[(3) - (3)].x);
		}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 955 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= (IS_SHADED | IS_FILLED);
		  (yyval.spec)->shaded = new char[strlen((yyvsp[(3) - (3)].lstr).str)+1];
		  strcpy((yyval.spec)->shaded, (yyvsp[(3) - (3)].lstr).str);
		}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 962 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= (IS_SHADED | IS_OUTLINED | IS_FILLED);
		  (yyval.spec)->shaded = new char[strlen((yyvsp[(3) - (3)].lstr).str)+1];
		  strcpy((yyval.spec)->shaded, (yyvsp[(3) - (3)].lstr).str);
		  (yyval.spec)->outlined = new char[strlen((yyvsp[(3) - (3)].lstr).str)+1];
		  strcpy((yyval.spec)->outlined, (yyvsp[(3) - (3)].lstr).str);
		}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 971 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= IS_OUTLINED;
		  (yyval.spec)->outlined = new char[strlen((yyvsp[(3) - (3)].lstr).str)+1];
		  strcpy((yyval.spec)->outlined, (yyvsp[(3) - (3)].lstr).str);
		}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 978 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  // line chop chop means line chop 0 chop 0
		  if ((yyval.spec)->flags & IS_DEFAULT_CHOPPED) {
		    (yyval.spec)->flags |= IS_CHOPPED;
		    (yyval.spec)->flags &= ~IS_DEFAULT_CHOPPED;
		    (yyval.spec)->start_chop = (yyval.spec)->end_chop = 0.0;
		  }
		  else if ((yyval.spec)->flags & IS_CHOPPED) {
		    (yyval.spec)->end_chop = 0.0;
		  }
		  else {
		    (yyval.spec)->flags |= IS_DEFAULT_CHOPPED;
		  }
		}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 994 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  if ((yyval.spec)->flags & IS_DEFAULT_CHOPPED) {
		    (yyval.spec)->flags |= IS_CHOPPED;
		    (yyval.spec)->flags &= ~IS_DEFAULT_CHOPPED;
		    (yyval.spec)->start_chop = 0.0;
		    (yyval.spec)->end_chop = (yyvsp[(3) - (3)].x);
		  }
		  else if ((yyval.spec)->flags & IS_CHOPPED) {
		    (yyval.spec)->end_chop = (yyvsp[(3) - (3)].x);
		  }
		  else {
		    (yyval.spec)->start_chop = (yyval.spec)->end_chop = (yyvsp[(3) - (3)].x);
		    (yyval.spec)->flags |= IS_CHOPPED;
		  }
		}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1011 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_SAME;
		}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1016 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_INVISIBLE;
		}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1021 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= HAS_LEFT_ARROW_HEAD;
		}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1026 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= HAS_RIGHT_ARROW_HEAD;
		}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1031 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= (HAS_LEFT_ARROW_HEAD|HAS_RIGHT_ARROW_HEAD);
		}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1036 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_CLOCKWISE;
		}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1041 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags &= ~IS_CLOCKWISE;
		}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1046 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  text_item **p;
		  for (p = & (yyval.spec)->text; *p; p = &(*p)->next)
		    ;
		  *p = new text_item((yyvsp[(2) - (2)].lstr).str, (yyvsp[(2) - (2)].lstr).filename, (yyvsp[(2) - (2)].lstr).lineno);
		}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1054 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  if ((yyval.spec)->text) {
		    text_item *p;
		    for (p = (yyval.spec)->text; p->next; p = p->next)
		      ;
		    p->adj.h = LEFT_ADJUST;
		  }
		}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1064 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  if ((yyval.spec)->text) {
		    text_item *p;
		    for (p = (yyval.spec)->text; p->next; p = p->next)
		      ;
		    p->adj.h = RIGHT_ADJUST;
		  }
		}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1074 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  if ((yyval.spec)->text) {
		    text_item *p;
		    for (p = (yyval.spec)->text; p->next; p = p->next)
		      ;
		    p->adj.v = ABOVE_ADJUST;
		  }
		}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1084 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  if ((yyval.spec)->text) {
		    text_item *p;
		    for (p = (yyval.spec)->text; p->next; p = p->next)
		      ;
		    p->adj.v = BELOW_ADJUST;
		  }
		}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1094 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (3)].spec);
		  (yyval.spec)->flags |= HAS_THICKNESS;
		  (yyval.spec)->thickness = (yyvsp[(3) - (3)].x);
		}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1100 "pic.y"
    {
		  (yyval.spec) = (yyvsp[(1) - (2)].spec);
		  (yyval.spec)->flags |= IS_ALIGNED;
		}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1108 "pic.y"
    { (yyval.lstr) = (yyvsp[(1) - (1)].lstr); }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1110 "pic.y"
    {
		  (yyval.lstr).filename = (yyvsp[(3) - (5)].lstr).filename;
		  (yyval.lstr).lineno = (yyvsp[(3) - (5)].lstr).lineno;
		  (yyval.lstr).str = do_sprintf((yyvsp[(3) - (5)].lstr).str, (yyvsp[(4) - (5)].dv).v, (yyvsp[(4) - (5)].dv).nv);
		  a_delete (yyvsp[(4) - (5)].dv).v;
		  a_delete (yyvsp[(3) - (5)].lstr).str;
		}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1121 "pic.y"
    {
		  (yyval.dv).v = 0;
		  (yyval.dv).nv = 0;
		  (yyval.dv).maxv = 0;
		}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1127 "pic.y"
    {
		  (yyval.dv) = (yyvsp[(1) - (3)].dv);
		  if ((yyval.dv).nv >= (yyval.dv).maxv) {
		    if ((yyval.dv).nv == 0) {
		      (yyval.dv).v = new double[4];
		      (yyval.dv).maxv = 4;
		    }
		    else {
		      double *oldv = (yyval.dv).v;
		      (yyval.dv).maxv *= 2;
#if 0
		      (yyval.dv).v = new double[(yyval.dv).maxv];
		      memcpy((yyval.dv).v, oldv, (yyval.dv).nv*sizeof(double));
#else
		      // workaround for bug in Compaq C++ V6.5-033
		      // for Compaq Tru64 UNIX V5.1A (Rev. 1885)
		      double *foo = new double[(yyval.dv).maxv];
		      memcpy(foo, oldv, (yyval.dv).nv*sizeof(double));
		      (yyval.dv).v = foo;
#endif
		      a_delete oldv;
		    }
		  }
		  (yyval.dv).v[(yyval.dv).nv] = (yyvsp[(3) - (3)].x);
		  (yyval.dv).nv += 1;
		}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1157 "pic.y"
    { (yyval.pair) = (yyvsp[(1) - (1)].pair); }
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1159 "pic.y"
    {
		  position pos = (yyvsp[(1) - (1)].pl);
		  (yyval.pair).x = pos.x;
		  (yyval.pair).y = pos.y;
		}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1165 "pic.y"
    {
		  position pos = (yyvsp[(2) - (3)].pl);
		  (yyval.pair).x = pos.x;
		  (yyval.pair).y = pos.y;
		}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1174 "pic.y"
    { (yyval.pair) = (yyvsp[(1) - (1)].pair); }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1176 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(1) - (3)].pair).x + (yyvsp[(3) - (3)].pair).x;
		  (yyval.pair).y = (yyvsp[(1) - (3)].pair).y + (yyvsp[(3) - (3)].pair).y;
		}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1181 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(2) - (5)].pair).x + (yyvsp[(4) - (5)].pair).x;
		  (yyval.pair).y = (yyvsp[(2) - (5)].pair).y + (yyvsp[(4) - (5)].pair).y;
		}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1186 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(1) - (3)].pair).x - (yyvsp[(3) - (3)].pair).x;
		  (yyval.pair).y = (yyvsp[(1) - (3)].pair).y - (yyvsp[(3) - (3)].pair).y;
		}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1191 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(2) - (5)].pair).x - (yyvsp[(4) - (5)].pair).x;
		  (yyval.pair).y = (yyvsp[(2) - (5)].pair).y - (yyvsp[(4) - (5)].pair).y;
		}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1196 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(2) - (5)].pair).x;
		  (yyval.pair).y = (yyvsp[(4) - (5)].pair).y;
		}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1201 "pic.y"
    {
		  (yyval.pair).x = (1.0 - (yyvsp[(1) - (5)].x))*(yyvsp[(3) - (5)].pair).x + (yyvsp[(1) - (5)].x)*(yyvsp[(5) - (5)].pair).x;
		  (yyval.pair).y = (1.0 - (yyvsp[(1) - (5)].x))*(yyvsp[(3) - (5)].pair).y + (yyvsp[(1) - (5)].x)*(yyvsp[(5) - (5)].pair).y;
		}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1206 "pic.y"
    {
		  (yyval.pair).x = (1.0 - (yyvsp[(2) - (7)].x))*(yyvsp[(4) - (7)].pair).x + (yyvsp[(2) - (7)].x)*(yyvsp[(6) - (7)].pair).x;
		  (yyval.pair).y = (1.0 - (yyvsp[(2) - (7)].x))*(yyvsp[(4) - (7)].pair).y + (yyvsp[(2) - (7)].x)*(yyvsp[(6) - (7)].pair).y;
		}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1212 "pic.y"
    {
		  (yyval.pair).x = (1.0 - (yyvsp[(1) - (6)].x))*(yyvsp[(3) - (6)].pair).x + (yyvsp[(1) - (6)].x)*(yyvsp[(5) - (6)].pair).x;
		  (yyval.pair).y = (1.0 - (yyvsp[(1) - (6)].x))*(yyvsp[(3) - (6)].pair).y + (yyvsp[(1) - (6)].x)*(yyvsp[(5) - (6)].pair).y;
		}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1217 "pic.y"
    {
		  (yyval.pair).x = (1.0 - (yyvsp[(2) - (8)].x))*(yyvsp[(4) - (8)].pair).x + (yyvsp[(2) - (8)].x)*(yyvsp[(6) - (8)].pair).x;
		  (yyval.pair).y = (1.0 - (yyvsp[(2) - (8)].x))*(yyvsp[(4) - (8)].pair).y + (yyvsp[(2) - (8)].x)*(yyvsp[(6) - (8)].pair).y;
		}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1230 "pic.y"
    {
		  (yyval.pair).x = (yyvsp[(1) - (3)].x);
		  (yyval.pair).y = (yyvsp[(3) - (3)].x);
		}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1235 "pic.y"
    { (yyval.pair) = (yyvsp[(2) - (3)].pair); }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1241 "pic.y"
    { (yyval.pl) = (yyvsp[(1) - (1)].pl); }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1243 "pic.y"
    {
		  path pth((yyvsp[(2) - (2)].crn));
		  if (!pth.follow((yyvsp[(1) - (2)].pl), & (yyval.pl)))
		    YYABORT;
		}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1249 "pic.y"
    {
		  path pth((yyvsp[(1) - (2)].crn));
		  if (!pth.follow((yyvsp[(2) - (2)].pl), & (yyval.pl)))
		    YYABORT;
		}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1255 "pic.y"
    {
		  path pth((yyvsp[(1) - (3)].crn));
		  if (!pth.follow((yyvsp[(3) - (3)].pl), & (yyval.pl)))
		    YYABORT;
		}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1261 "pic.y"
    {
		  (yyval.pl).x = current_position.x;
		  (yyval.pl).y = current_position.y;
		  (yyval.pl).obj = 0;
		}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1270 "pic.y"
    {
		  place *p = lookup_label((yyvsp[(1) - (1)].str));
		  if (!p) {
		    lex_error("there is no place `%1'", (yyvsp[(1) - (1)].str));
		    YYABORT;
		  }
		  (yyval.pl) = *p;
		  a_delete (yyvsp[(1) - (1)].str);
		}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1280 "pic.y"
    { (yyval.pl).obj = (yyvsp[(1) - (1)].obj); }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1282 "pic.y"
    {
		  path pth((yyvsp[(3) - (3)].str));
		  if (!pth.follow((yyvsp[(1) - (3)].pl), & (yyval.pl)))
		    YYABORT;
		}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1291 "pic.y"
    { (yyval.n) = (yyvsp[(1) - (1)].n); }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1293 "pic.y"
    {
		  // XXX Check for overflow (and non-integers?).
		  (yyval.n) = (int)(yyvsp[(2) - (3)].x);
		}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1301 "pic.y"
    { (yyval.n) = 1; }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1303 "pic.y"
    { (yyval.n) = (yyvsp[(1) - (2)].n); }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1308 "pic.y"
    {
		  int count = 0;
		  object *p;
		  for (p = olist.head; p != 0; p = p->next)
		    if (p->type() == (yyvsp[(2) - (2)].obtype) && ++count == (yyvsp[(1) - (2)].n)) {
		      (yyval.obj) = p;
		      break;
		    }
		  if (p == 0) {
		    lex_error("there is no %1%2 %3", (yyvsp[(1) - (2)].n), ordinal_postfix((yyvsp[(1) - (2)].n)),
			      object_type_name((yyvsp[(2) - (2)].obtype)));
		    YYABORT;
		  }
		}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1323 "pic.y"
    {
		  int count = 0;
		  object *p;
		  for (p = olist.tail; p != 0; p = p->prev)
		    if (p->type() == (yyvsp[(2) - (2)].obtype) && ++count == (yyvsp[(1) - (2)].n)) {
		      (yyval.obj) = p;
		      break;
		    }
		  if (p == 0) {
		    lex_error("there is no %1%2 last %3", (yyvsp[(1) - (2)].n),
			      ordinal_postfix((yyvsp[(1) - (2)].n)), object_type_name((yyvsp[(2) - (2)].obtype)));
		    YYABORT;
		  }
		}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1341 "pic.y"
    { (yyval.obtype) = BOX_OBJECT; }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1343 "pic.y"
    { (yyval.obtype) = CIRCLE_OBJECT; }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1345 "pic.y"
    { (yyval.obtype) = ELLIPSE_OBJECT; }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1347 "pic.y"
    { (yyval.obtype) = ARC_OBJECT; }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1349 "pic.y"
    { (yyval.obtype) = LINE_OBJECT; }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1351 "pic.y"
    { (yyval.obtype) = ARROW_OBJECT; }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1353 "pic.y"
    { (yyval.obtype) = SPLINE_OBJECT; }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1355 "pic.y"
    { (yyval.obtype) = BLOCK_OBJECT; }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1357 "pic.y"
    { (yyval.obtype) = TEXT_OBJECT; }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1362 "pic.y"
    { (yyval.pth) = new path((yyvsp[(2) - (2)].str)); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1364 "pic.y"
    {
		  (yyval.pth) = (yyvsp[(1) - (3)].pth);
		  (yyval.pth)->append((yyvsp[(3) - (3)].str));
		}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1372 "pic.y"
    { (yyval.pth) = new path((yyvsp[(1) - (1)].crn)); }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1376 "pic.y"
    { (yyval.pth) = (yyvsp[(1) - (1)].pth); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1378 "pic.y"
    {
		  (yyval.pth) = (yyvsp[(1) - (2)].pth);
		  (yyval.pth)->append((yyvsp[(2) - (2)].crn));
		}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1386 "pic.y"
    { (yyval.pth) = (yyvsp[(1) - (1)].pth); }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1388 "pic.y"
    {
		  (yyval.pth) = (yyvsp[(2) - (5)].pth);
		  (yyval.pth)->set_ypath((yyvsp[(4) - (5)].pth));
		}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1394 "pic.y"
    {
		  lex_warning("`%1%2 last %3' in `with' argument ignored",
			      (yyvsp[(1) - (4)].n), ordinal_postfix((yyvsp[(1) - (4)].n)), object_type_name((yyvsp[(3) - (4)].obtype)));
		  (yyval.pth) = (yyvsp[(4) - (4)].pth);
		}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1400 "pic.y"
    {
		  lex_warning("`last %1' in `with' argument ignored",
			      object_type_name((yyvsp[(2) - (3)].obtype)));
		  (yyval.pth) = (yyvsp[(3) - (3)].pth);
		}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1406 "pic.y"
    {
		  lex_warning("`%1%2 %3' in `with' argument ignored",
			      (yyvsp[(1) - (3)].n), ordinal_postfix((yyvsp[(1) - (3)].n)), object_type_name((yyvsp[(2) - (3)].obtype)));
		  (yyval.pth) = (yyvsp[(3) - (3)].pth);
		}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1412 "pic.y"
    {
		  lex_warning("initial `%1' in `with' argument ignored", (yyvsp[(1) - (2)].str));
		  a_delete (yyvsp[(1) - (2)].str);
		  (yyval.pth) = (yyvsp[(2) - (2)].pth);
		}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1421 "pic.y"
    { (yyval.crn) = &object::north; }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1423 "pic.y"
    { (yyval.crn) = &object::east; }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1425 "pic.y"
    { (yyval.crn) = &object::west; }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1427 "pic.y"
    { (yyval.crn) = &object::south; }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1429 "pic.y"
    { (yyval.crn) = &object::north_east; }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1431 "pic.y"
    { (yyval.crn) = &object:: south_east; }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1433 "pic.y"
    { (yyval.crn) = &object::north_west; }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1435 "pic.y"
    { (yyval.crn) = &object::south_west; }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1437 "pic.y"
    { (yyval.crn) = &object::center; }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1439 "pic.y"
    { (yyval.crn) = &object::start; }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1441 "pic.y"
    { (yyval.crn) = &object::end; }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1443 "pic.y"
    { (yyval.crn) = &object::north; }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1445 "pic.y"
    { (yyval.crn) = &object::south; }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1447 "pic.y"
    { (yyval.crn) = &object::west; }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1449 "pic.y"
    { (yyval.crn) = &object::east; }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1451 "pic.y"
    { (yyval.crn) = &object::north_west; }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1453 "pic.y"
    { (yyval.crn) = &object::south_west; }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1455 "pic.y"
    { (yyval.crn) = &object::north_east; }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1457 "pic.y"
    { (yyval.crn) = &object::south_east; }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1459 "pic.y"
    { (yyval.crn) = &object::west; }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1461 "pic.y"
    { (yyval.crn) = &object::east; }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1463 "pic.y"
    { (yyval.crn) = &object::north_west; }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1465 "pic.y"
    { (yyval.crn) = &object::south_west; }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1467 "pic.y"
    { (yyval.crn) = &object::north_east; }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1469 "pic.y"
    { (yyval.crn) = &object::south_east; }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1471 "pic.y"
    { (yyval.crn) = &object::north; }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1473 "pic.y"
    { (yyval.crn) = &object::south; }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1475 "pic.y"
    { (yyval.crn) = &object::east; }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1477 "pic.y"
    { (yyval.crn) = &object::west; }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1479 "pic.y"
    { (yyval.crn) = &object::center; }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1481 "pic.y"
    { (yyval.crn) = &object::start; }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1483 "pic.y"
    { (yyval.crn) = &object::end; }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1488 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (1)].x); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1490 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (1)].x); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1495 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) < (yyvsp[(3) - (3)].x)); }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1500 "pic.y"
    {
		  if (!lookup_variable((yyvsp[(1) - (1)].str), & (yyval.x))) {
		    lex_error("there is no variable `%1'", (yyvsp[(1) - (1)].str));
		    YYABORT;
		  }
		  a_delete (yyvsp[(1) - (1)].str);
		}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1508 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (1)].x); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1510 "pic.y"
    {
		  if ((yyvsp[(1) - (2)].pl).obj != 0)
		    (yyval.x) = (yyvsp[(1) - (2)].pl).obj->origin().x;
		  else
		    (yyval.x) = (yyvsp[(1) - (2)].pl).x;
		}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1517 "pic.y"
    {
		  if ((yyvsp[(1) - (2)].pl).obj != 0)
		    (yyval.x) = (yyvsp[(1) - (2)].pl).obj->origin().y;
		  else
		    (yyval.x) = (yyvsp[(1) - (2)].pl).y;
		}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1524 "pic.y"
    {
		  if ((yyvsp[(1) - (2)].pl).obj != 0)
		    (yyval.x) = (yyvsp[(1) - (2)].pl).obj->height();
		  else
		    (yyval.x) = 0.0;
		}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1531 "pic.y"
    {
		  if ((yyvsp[(1) - (2)].pl).obj != 0)
		    (yyval.x) = (yyvsp[(1) - (2)].pl).obj->width();
		  else
		    (yyval.x) = 0.0;
		}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1538 "pic.y"
    {
		  if ((yyvsp[(1) - (2)].pl).obj != 0)
		    (yyval.x) = (yyvsp[(1) - (2)].pl).obj->radius();
		  else
		    (yyval.x) = 0.0;
		}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1545 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (3)].x) + (yyvsp[(3) - (3)].x); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1547 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (3)].x) - (yyvsp[(3) - (3)].x); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1549 "pic.y"
    { (yyval.x) = (yyvsp[(1) - (3)].x) * (yyvsp[(3) - (3)].x); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1551 "pic.y"
    {
		  if ((yyvsp[(3) - (3)].x) == 0.0) {
		    lex_error("division by zero");
		    YYABORT;
		  }
		  (yyval.x) = (yyvsp[(1) - (3)].x)/(yyvsp[(3) - (3)].x);
		}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1559 "pic.y"
    {
		  if ((yyvsp[(3) - (3)].x) == 0.0) {
		    lex_error("modulus by zero");
		    YYABORT;
		  }
		  (yyval.x) = fmod((yyvsp[(1) - (3)].x), (yyvsp[(3) - (3)].x));
		}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1567 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = pow((yyvsp[(1) - (3)].x), (yyvsp[(3) - (3)].x));
		  if (errno == EDOM) {
		    lex_error("arguments to `^' operator out of domain");
		    YYABORT;
		  }
		  if (errno == ERANGE) {
		    lex_error("result of `^' operator out of range");
		    YYABORT;
		  }
		}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1580 "pic.y"
    { (yyval.x) = -(yyvsp[(2) - (2)].x); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1582 "pic.y"
    { (yyval.x) = (yyvsp[(2) - (3)].x); }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1584 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = sin((yyvsp[(3) - (4)].x));
		  if (errno == ERANGE) {
		    lex_error("sin result out of range");
		    YYABORT;
		  }
		}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1593 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = cos((yyvsp[(3) - (4)].x));
		  if (errno == ERANGE) {
		    lex_error("cos result out of range");
		    YYABORT;
		  }
		}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1602 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = atan2((yyvsp[(3) - (6)].x), (yyvsp[(5) - (6)].x));
		  if (errno == EDOM) {
		    lex_error("atan2 argument out of domain");
		    YYABORT;
		  }
		  if (errno == ERANGE) {
		    lex_error("atan2 result out of range");
		    YYABORT;
		  }
		}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1615 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = log10((yyvsp[(3) - (4)].x));
		  if (errno == ERANGE) {
		    lex_error("log result out of range");
		    YYABORT;
		  }
		}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1624 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = pow(10.0, (yyvsp[(3) - (4)].x));
		  if (errno == ERANGE) {
		    lex_error("exp result out of range");
		    YYABORT;
		  }
		}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1633 "pic.y"
    {
		  errno = 0;
		  (yyval.x) = sqrt((yyvsp[(3) - (4)].x));
		  if (errno == EDOM) {
		    lex_error("sqrt argument out of domain");
		    YYABORT;
		  }
		}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1642 "pic.y"
    { (yyval.x) = (yyvsp[(3) - (6)].x) > (yyvsp[(5) - (6)].x) ? (yyvsp[(3) - (6)].x) : (yyvsp[(5) - (6)].x); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1644 "pic.y"
    { (yyval.x) = (yyvsp[(3) - (6)].x) < (yyvsp[(5) - (6)].x) ? (yyvsp[(3) - (6)].x) : (yyvsp[(5) - (6)].x); }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1646 "pic.y"
    { (yyval.x) = (yyvsp[(3) - (4)].x) < 0 ? -floor(-(yyvsp[(3) - (4)].x)) : floor((yyvsp[(3) - (4)].x)); }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1648 "pic.y"
    { (yyval.x) = 1.0 + floor(((rand()&0x7fff)/double(0x7fff))*(yyvsp[(3) - (4)].x)); }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1650 "pic.y"
    {
		  /* return a random number in the range [0,1) */
		  /* portable, but not very random */
		  (yyval.x) = (rand() & 0x7fff) / double(0x8000);
		}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1656 "pic.y"
    {
		  (yyval.x) = 0;
		  srand((unsigned int)(yyvsp[(3) - (4)].x));
		}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1661 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) <= (yyvsp[(3) - (3)].x)); }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1663 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) > (yyvsp[(3) - (3)].x)); }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1665 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) >= (yyvsp[(3) - (3)].x)); }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1667 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) == (yyvsp[(3) - (3)].x)); }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1669 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != (yyvsp[(3) - (3)].x)); }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1671 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 && (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1673 "pic.y"
    { (yyval.x) = ((yyvsp[(1) - (3)].x) != 0.0 || (yyvsp[(3) - (3)].x) != 0.0); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1675 "pic.y"
    { (yyval.x) = ((yyvsp[(2) - (2)].x) == 0.0); }
    break;



/* Line 1455 of yacc.c  */
#line 4999 "pic.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1679 "pic.y"


/* bison defines const to be empty unless __STDC__ is defined, which it
isn't under cfront */

#ifdef const
#undef const
#endif

static struct {
  const char *name;
  double val;
  int scaled;		     // non-zero if val should be multiplied by scale
} defaults_table[] = {
  { "arcrad", .25, 1 },
  { "arrowht", .1, 1 },
  { "arrowwid", .05, 1 },
  { "circlerad", .25, 1 },
  { "boxht", .5, 1 },
  { "boxwid", .75, 1 },
  { "boxrad", 0.0, 1 },
  { "dashwid", .05, 1 },
  { "ellipseht", .5, 1 },
  { "ellipsewid", .75, 1 },
  { "moveht", .5, 1 },
  { "movewid", .5, 1 },
  { "lineht", .5, 1 },
  { "linewid", .5, 1 },
  { "textht", 0.0, 1 },
  { "textwid", 0.0, 1 },
  { "scale", 1.0, 0 },
  { "linethick", -1.0, 0 },		// in points
  { "fillval", .5, 0 },
  { "arrowhead", 1.0, 0 },
  { "maxpswid", 8.5, 0 },
  { "maxpsht", 11.0, 0 },
};

place *lookup_label(const char *label)
{
  saved_state *state = current_saved_state;
  PTABLE(place) *tbl = current_table;
  for (;;) {
    place *pl = tbl->lookup(label);
    if (pl)
      return pl;
    if (!state)
      return 0;
    tbl = state->tbl;
    state = state->prev;
  }
}

void define_label(const char *label, const place *pl)
{
  place *p = new place[1];
  *p = *pl;
  current_table->define(label, p);
}

int lookup_variable(const char *name, double *val)
{
  place *pl = lookup_label(name);
  if (pl) {
    *val = pl->x;
    return 1;
  }
  return 0;
}

void define_variable(const char *name, double val)
{
  place *p = new place[1];
  p->obj = 0;
  p->x = val;
  p->y = 0.0;
  current_table->define(name, p);
  if (strcmp(name, "scale") == 0) {
    // When the scale changes, reset all scaled pre-defined variables to
    // their default values.
    for (unsigned int i = 0;
	 i < sizeof(defaults_table)/sizeof(defaults_table[0]); i++) 
      if (defaults_table[i].scaled)
	define_variable(defaults_table[i].name, val*defaults_table[i].val);
  }
}

// called once only (not once per parse)

void parse_init()
{
  current_direction = RIGHT_DIRECTION;
  current_position.x = 0.0;
  current_position.y = 0.0;
  // This resets everything to its default value.
  reset_all();
}

void reset(const char *nm)
{
  for (unsigned int i = 0;
       i < sizeof(defaults_table)/sizeof(defaults_table[0]); i++)
    if (strcmp(nm, defaults_table[i].name) == 0) {
      double val = defaults_table[i].val;
      if (defaults_table[i].scaled) {
	double scale;
	lookup_variable("scale", &scale);
	val *= scale;
      }
      define_variable(defaults_table[i].name, val);
      return;
    }
  lex_error("`%1' is not a predefined variable", nm);
}

void reset_all()
{
  // We only have to explicitly reset the pre-defined variables that
  // aren't scaled because `scale' is not scaled, and changing the
  // value of `scale' will reset all the pre-defined variables that
  // are scaled.
  for (unsigned int i = 0;
       i < sizeof(defaults_table)/sizeof(defaults_table[0]); i++)
    if (!defaults_table[i].scaled)
      define_variable(defaults_table[i].name, defaults_table[i].val);
}

// called after each parse

void parse_cleanup()
{
  while (current_saved_state != 0) {
    delete current_table;
    current_table = current_saved_state->tbl;
    saved_state *tem = current_saved_state;
    current_saved_state = current_saved_state->prev;
    delete tem;
  }
  assert(current_table == &top_table);
  PTABLE_ITERATOR(place) iter(current_table);
  const char *key;
  place *pl;
  while (iter.next(&key, &pl))
    if (pl->obj != 0) {
      position pos = pl->obj->origin();
      pl->obj = 0;
      pl->x = pos.x;
      pl->y = pos.y;
    }
  while (olist.head != 0) {
    object *tem = olist.head;
    olist.head = olist.head->next;
    delete tem;
  }
  olist.tail = 0;
  current_direction = RIGHT_DIRECTION;
  current_position.x = 0.0;
  current_position.y = 0.0;
}

const char *ordinal_postfix(int n)
{
  if (n < 10 || n > 20)
    switch (n % 10) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    }
  return "th";
}

const char *object_type_name(object_type type)
{
  switch (type) {
  case BOX_OBJECT:
    return "box";
  case CIRCLE_OBJECT:
    return "circle";
  case ELLIPSE_OBJECT:
    return "ellipse";
  case ARC_OBJECT:
    return "arc";
  case SPLINE_OBJECT:
    return "spline";
  case LINE_OBJECT:
    return "line";
  case ARROW_OBJECT:
    return "arrow";
  case MOVE_OBJECT:
    return "move";
  case TEXT_OBJECT:
    return "\"\"";
  case BLOCK_OBJECT:
    return "[]";
  case OTHER_OBJECT:
  case MARK_OBJECT:
  default:
    break;
  }
  return "object";
}

static char sprintf_buf[1024];

char *format_number(const char *form, double n)
{
  if (form == 0)
    form = "%g";
  return do_sprintf(form, &n, 1);
}

char *do_sprintf(const char *form, const double *v, int nv)
{
  string result;
  int i = 0;
  string one_format;
  while (*form) {
    if (*form == '%') {
      one_format += *form++;
      for (; *form != '\0' && strchr("#-+ 0123456789.", *form) != 0; form++)
	one_format += *form;
      if (*form == '\0' || strchr("eEfgG%", *form) == 0) {
	lex_error("bad sprintf format");
	result += one_format;
	result += form;
	break;
      }
      if (*form == '%') {
	one_format += *form++;
	one_format += '\0';
	snprintf(sprintf_buf, sizeof(sprintf_buf),
		 "%s", one_format.contents());
      }
      else {
	if (i >= nv) {
	  lex_error("too few arguments to snprintf");
	  result += one_format;
	  result += form;
	  break;
	}
	one_format += *form++;
	one_format += '\0';
	snprintf(sprintf_buf, sizeof(sprintf_buf),
		 one_format.contents(), v[i++]);
      }
      one_format.clear();
      result += sprintf_buf;
    }
    else
      result += *form++;
  }
  result += '\0';
  return strsave(result.contents());
}

