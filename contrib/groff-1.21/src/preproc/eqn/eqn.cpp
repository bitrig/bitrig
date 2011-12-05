
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
#line 19 "eqn.y"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib.h"
#include "box.h"
extern int non_empty_flag;
int yylex();
void yyerror(const char *);


/* Line 189 of yacc.c  */
#line 86 "eqn.cpp"

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

/* Line 214 of yacc.c  */
#line 31 "eqn.y"

	char *str;
	box *b;
	pile_box *pb;
	matrix_box *mb;
	int n;
	column *col;



/* Line 214 of yacc.c  */
#line 253 "eqn.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 265 "eqn.cpp"

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
#define YYFINAL  72
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   379

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  75
/* YYNRULES -- Number of states.  */
#define YYNSTATES  142

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   315

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,    63,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    61,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,    65,    62,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    11,    13,    16,    19,
      21,    25,    29,    35,    41,    43,    46,    50,    54,    56,
      60,    62,    66,    72,    74,    76,    79,    82,    84,    86,
      88,    92,    95,    98,   101,   104,   109,   115,   119,   122,
     125,   128,   132,   136,   139,   142,   145,   148,   152,   156,
     160,   164,   168,   172,   176,   179,   183,   185,   187,   191,
     195,   200,   202,   205,   207,   211,   215,   220,   223,   226,
     229,   232,   234,   236,   238,   240
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      67,     0,    -1,    -1,    68,    -1,    69,    -1,    68,    69,
      -1,    70,    -1,    37,    69,    -1,    38,    69,    -1,    71,
      -1,    71,    14,    70,    -1,    71,    15,    71,    -1,    71,
      15,    71,    14,    70,    -1,    71,    15,    71,    15,    70,
      -1,    72,    -1,     5,    71,    -1,    71,     3,    71,    -1,
      71,     4,    71,    -1,    73,    -1,    74,     7,    72,    -1,
      74,    -1,    74,     6,    73,    -1,    74,     6,    74,     7,
      72,    -1,    26,    -1,    27,    -1,    42,    27,    -1,    43,
      26,    -1,    61,    -1,    62,    -1,    63,    -1,    64,    68,
      65,    -1,    11,    77,    -1,     8,    77,    -1,     9,    77,
      -1,    10,    77,    -1,    32,    64,    78,    65,    -1,    12,
      83,    68,    13,    83,    -1,    12,    83,    68,    -1,    74,
      23,    -1,    74,    24,    -1,    74,    41,    -1,    74,    22,
      74,    -1,    74,    44,    74,    -1,    18,    74,    -1,    19,
      74,    -1,    20,    74,    -1,    21,    74,    -1,    17,    82,
      74,    -1,    16,    82,    74,    -1,    28,    75,    74,    -1,
      29,    75,    74,    -1,    31,    75,    74,    -1,    30,    75,
      74,    -1,    39,    82,    74,    -1,    40,    74,    -1,    45,
      82,    74,    -1,    82,    -1,    68,    -1,    76,    25,    68,
      -1,    64,    76,    65,    -1,    75,    64,    76,    65,    -1,
      81,    -1,    78,    81,    -1,    68,    -1,    79,    25,    68,
      -1,    64,    79,    65,    -1,    75,    64,    79,    65,    -1,
      33,    80,    -1,    34,    80,    -1,    35,    80,    -1,    36,
      80,    -1,    26,    -1,    27,    -1,    82,    -1,    64,    -1,
      65,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   122,   122,   124,   129,   131,   142,   144,   146,   151,
     153,   155,   157,   159,   164,   166,   168,   170,   175,   177,
     182,   184,   186,   191,   193,   195,   197,   199,   201,   203,
     205,   207,   209,   211,   213,   215,   217,   219,   221,   223,
     225,   227,   229,   231,   233,   235,   237,   239,   241,   243,
     245,   247,   249,   251,   253,   255,   260,   270,   272,   277,
     279,   284,   286,   291,   293,   298,   300,   305,   307,   309,
     311,   315,   317,   322,   324,   326
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "OVER", "SMALLOVER", "SQRT", "SUB",
  "SUP", "LPILE", "RPILE", "CPILE", "PILE", "LEFT", "RIGHT", "TO", "FROM",
  "SIZE", "FONT", "ROMAN", "BOLD", "ITALIC", "FAT", "ACCENT", "BAR",
  "UNDER", "ABOVE", "TEXT", "QUOTED_TEXT", "FWD", "BACK", "DOWN", "UP",
  "MATRIX", "COL", "LCOL", "RCOL", "CCOL", "MARK", "LINEUP", "TYPE",
  "VCENTER", "PRIME", "SPLIT", "NOSPLIT", "UACCENT", "SPECIAL", "SPACE",
  "GFONT", "GSIZE", "DEFINE", "NDEFINE", "TDEFINE", "SDEFINE", "UNDEF",
  "IFDEF", "INCLUDE", "DELIM", "CHARTYPE", "SET", "GRFONT", "GBFONT",
  "'^'", "'~'", "'\\t'", "'{'", "'}'", "$accept", "top", "equation",
  "mark", "from_to", "sqrt_over", "script", "nonsup", "simple", "number",
  "pile_element_list", "pile_arg", "column_list", "column_element_list",
  "column_arg", "column", "text", "delim", 0
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
     315,    94,   126,     9,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    69,    69,    69,    70,
      70,    70,    70,    70,    71,    71,    71,    71,    72,    72,
      73,    73,    73,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    75,    76,    76,    77,
      77,    78,    78,    79,    79,    80,    80,    81,    81,    81,
      81,    82,    82,    83,    83,    83
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     2,     1,     2,     2,     1,
       3,     3,     5,     5,     1,     2,     3,     3,     1,     3,
       1,     3,     5,     1,     1,     2,     2,     1,     1,     1,
       3,     2,     2,     2,     2,     4,     5,     3,     2,     2,
       2,     3,     3,     2,     2,     2,     2,     3,     3,     3,
       3,     3,     3,     3,     2,     3,     1,     1,     3,     3,
       4,     1,     2,     1,     3,     3,     4,     2,     2,     2,
       2,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,    24,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    27,    28,    29,
       0,     0,     3,     4,     6,     9,    14,    18,    20,    15,
      71,    72,     0,     0,    32,    56,    33,    34,    31,    74,
      75,    73,     0,     0,     0,    43,    44,    45,    46,     0,
       0,     0,     0,     0,     7,     8,     0,    54,    25,    26,
       0,     0,     1,     5,     0,     0,     0,     0,     0,     0,
       0,    38,    39,    40,     0,    57,     0,     0,    37,    48,
      47,    49,    50,    52,    51,     0,     0,     0,     0,     0,
      61,    53,    55,    30,    16,    17,    10,    11,    21,    20,
      19,    41,    42,     0,    59,     0,     0,     0,     0,    67,
      68,    69,    70,    35,    62,     0,     0,     0,    58,    60,
      36,    63,     0,     0,    12,    13,    22,     0,    65,     0,
      64,    66
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    31,    85,    33,    34,    35,    36,    37,    38,    43,
      86,    44,    99,   132,   119,   100,    45,    52
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -76
static const yytype_int16 yypact[] =
{
     230,   269,     6,     6,     6,     6,     2,    14,    14,   308,
     308,   308,   308,   -76,   -76,    14,    14,    14,    14,   -50,
     230,   230,    14,   308,     4,    23,    14,   -76,   -76,   -76,
     230,    24,   230,   -76,   -76,    70,   -76,   -76,    20,   -76,
     -76,   -76,   230,   -44,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,   230,   308,   308,    57,    57,    57,    57,   308,
     308,   308,   308,     3,   -76,   -76,   308,    57,   -76,   -76,
     308,   130,   -76,   -76,   269,   269,   269,   269,   308,   308,
     308,   -76,   -76,   -76,   308,   230,   -12,   230,   191,    57,
      57,    57,    57,    57,    57,     8,     8,     8,     8,    12,
     -76,    57,    57,   -76,   -76,   -76,   -76,    79,   -76,   335,
     -76,   -76,   -76,   230,   -76,    -6,     2,   230,    28,   -76,
     -76,   -76,   -76,   -76,   -76,   269,   269,   308,   230,   -76,
     -76,   230,    -3,   230,   -76,   -76,   -76,   230,   -76,    -2,
     230,   -76
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -76,   -76,     0,   -17,   -75,     1,   -67,   -13,    46,    -7,
       9,    13,   -76,   -47,    22,    -4,    -1,   -29
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      32,   106,    39,    64,    65,    51,    53,    54,    59,    60,
      61,    62,   110,   113,    63,    73,    46,    47,    48,   113,
      87,    66,   137,   137,    72,    70,    78,    79,    40,    41,
      71,    68,    40,    41,    40,    41,    95,    96,    97,    98,
      40,    41,    80,    81,    82,    95,    96,    97,    98,    69,
     134,   135,    88,   114,    73,    55,    56,    57,    58,   129,
     136,    83,   138,   141,    84,   108,    49,    50,    73,    67,
      42,    73,   117,    74,    75,   104,   105,   123,   107,    80,
      81,    82,    74,    75,    76,    77,   139,   130,   118,   118,
     118,   118,   133,   125,   126,   124,   115,     0,    83,    89,
      90,    84,     0,     0,     0,    91,    92,    93,    94,     0,
       0,    73,   101,   128,    73,    51,   102,   131,   120,   121,
     122,     0,     0,    73,   109,     0,   111,     0,     0,     0,
     112,     0,     0,   131,     0,     1,     0,   140,     2,     3,
       4,     5,     6,     0,     0,     0,     7,     8,     9,    10,
      11,    12,     0,     0,     0,     0,    13,    14,    15,    16,
      17,    18,    19,     0,     0,     0,     0,    20,    21,    22,
      23,     0,    24,    25,     0,    26,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    27,    28,    29,    30,   103,     1,     0,     0,     2,
       3,     4,     5,     6,   116,     0,     0,     7,     8,     9,
      10,    11,    12,     0,     0,     0,     0,    13,    14,    15,
      16,    17,    18,    19,     0,     0,     0,     0,    20,    21,
      22,    23,     0,    24,    25,     1,    26,     0,     2,     3,
       4,     5,     6,     0,     0,     0,     7,     8,     9,    10,
      11,    12,    27,    28,    29,    30,    13,    14,    15,    16,
      17,    18,    19,     0,     0,     0,     0,    20,    21,    22,
      23,     0,    24,    25,     1,    26,     0,     2,     3,     4,
       5,     6,     0,     0,     0,     7,     8,     9,    10,    11,
      12,    27,    28,    29,    30,    13,    14,    15,    16,    17,
      18,    19,     0,     0,     0,     0,     0,     0,    22,    23,
       0,    24,    25,     0,    26,     0,     2,     3,     4,     5,
       6,     0,     0,     0,     7,     8,     9,    10,    11,    12,
      27,    28,    29,    30,    13,    14,    15,    16,    17,    18,
      19,    78,   127,     0,     0,     0,     0,    22,    23,     0,
      24,    25,     0,    26,     0,     0,     0,    80,    81,    82,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
      28,    29,    30,     0,     0,     0,    83,     0,     0,    84
};

static const yytype_int16 yycheck[] =
{
       0,    76,     1,    20,    21,     6,     7,     8,    15,    16,
      17,    18,    79,    25,    64,    32,     3,     4,     5,    25,
      64,    22,    25,    25,     0,    26,     6,     7,    26,    27,
      30,    27,    26,    27,    26,    27,    33,    34,    35,    36,
      26,    27,    22,    23,    24,    33,    34,    35,    36,    26,
     125,   126,    52,    65,    71,     9,    10,    11,    12,    65,
     127,    41,    65,    65,    44,    78,    64,    65,    85,    23,
      64,    88,    64,     3,     4,    74,    75,    65,    77,    22,
      23,    24,     3,     4,    14,    15,   133,   116,    95,    96,
      97,    98,    64,    14,    15,    99,    87,    -1,    41,    53,
      54,    44,    -1,    -1,    -1,    59,    60,    61,    62,    -1,
      -1,   128,    66,   113,   131,   116,    70,   117,    96,    97,
      98,    -1,    -1,   140,    78,    -1,    80,    -1,    -1,    -1,
      84,    -1,    -1,   133,    -1,     5,    -1,   137,     8,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    21,    -1,    -1,    -1,    -1,    26,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    -1,    42,    43,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    61,    62,    63,    64,    65,     5,    -1,    -1,     8,
       9,    10,    11,    12,    13,    -1,    -1,    16,    17,    18,
      19,    20,    21,    -1,    -1,    -1,    -1,    26,    27,    28,
      29,    30,    31,    32,    -1,    -1,    -1,    -1,    37,    38,
      39,    40,    -1,    42,    43,     5,    45,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    21,    61,    62,    63,    64,    26,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    -1,    42,    43,     5,    45,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    20,
      21,    61,    62,    63,    64,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,
      -1,    42,    43,    -1,    45,    -1,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,    21,
      61,    62,    63,    64,    26,    27,    28,    29,    30,    31,
      32,     6,     7,    -1,    -1,    -1,    -1,    39,    40,    -1,
      42,    43,    -1,    45,    -1,    -1,    -1,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,
      62,    63,    64,    -1,    -1,    -1,    41,    -1,    -1,    44
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,     8,     9,    10,    11,    12,    16,    17,    18,
      19,    20,    21,    26,    27,    28,    29,    30,    31,    32,
      37,    38,    39,    40,    42,    43,    45,    61,    62,    63,
      64,    67,    68,    69,    70,    71,    72,    73,    74,    71,
      26,    27,    64,    75,    77,    82,    77,    77,    77,    64,
      65,    82,    83,    82,    82,    74,    74,    74,    74,    75,
      75,    75,    75,    64,    69,    69,    82,    74,    27,    26,
      82,    68,     0,    69,     3,     4,    14,    15,     6,     7,
      22,    23,    24,    41,    44,    68,    76,    64,    68,    74,
      74,    74,    74,    74,    74,    33,    34,    35,    36,    78,
      81,    74,    74,    65,    71,    71,    70,    71,    73,    74,
      72,    74,    74,    25,    65,    76,    13,    64,    75,    80,
      80,    80,    80,    65,    81,    14,    15,     7,    68,    65,
      83,    68,    79,    64,    70,    70,    72,    25,    65,    79,
      68,    65
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
#line 125 "eqn.y"
    { (yyvsp[(1) - (1)].b)->top_level(); non_empty_flag = 1; }
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 130 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 132 "eqn.y"
    {
		  list_box *lb = (yyvsp[(1) - (2)].b)->to_list_box();
		  if (!lb)
		    lb = new list_box((yyvsp[(1) - (2)].b));
		  lb->append((yyvsp[(2) - (2)].b));
		  (yyval.b) = lb;
		}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 143 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 145 "eqn.y"
    { (yyval.b) = make_mark_box((yyvsp[(2) - (2)].b)); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 147 "eqn.y"
    { (yyval.b) = make_lineup_box((yyvsp[(2) - (2)].b)); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 152 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 154 "eqn.y"
    { (yyval.b) = make_limit_box((yyvsp[(1) - (3)].b), 0, (yyvsp[(3) - (3)].b)); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 156 "eqn.y"
    { (yyval.b) = make_limit_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b), 0); }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 158 "eqn.y"
    { (yyval.b) = make_limit_box((yyvsp[(1) - (5)].b), (yyvsp[(3) - (5)].b), (yyvsp[(5) - (5)].b)); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 160 "eqn.y"
    { (yyval.b) = make_limit_box((yyvsp[(1) - (5)].b), make_limit_box((yyvsp[(3) - (5)].b), (yyvsp[(5) - (5)].b), 0), 0); }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 165 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 167 "eqn.y"
    { (yyval.b) = make_sqrt_box((yyvsp[(2) - (2)].b)); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 169 "eqn.y"
    { (yyval.b) = make_over_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b)); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 171 "eqn.y"
    { (yyval.b) = make_small_over_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b)); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 176 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 178 "eqn.y"
    { (yyval.b) = make_script_box((yyvsp[(1) - (3)].b), 0, (yyvsp[(3) - (3)].b)); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 183 "eqn.y"
    { (yyval.b) = (yyvsp[(1) - (1)].b); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 185 "eqn.y"
    { (yyval.b) = make_script_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b), 0); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 187 "eqn.y"
    { (yyval.b) = make_script_box((yyvsp[(1) - (5)].b), (yyvsp[(3) - (5)].b), (yyvsp[(5) - (5)].b)); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 192 "eqn.y"
    { (yyval.b) = split_text((yyvsp[(1) - (1)].str)); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 194 "eqn.y"
    { (yyval.b) = new quoted_text_box((yyvsp[(1) - (1)].str)); }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 196 "eqn.y"
    { (yyval.b) = split_text((yyvsp[(2) - (2)].str)); }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 198 "eqn.y"
    { (yyval.b) = new quoted_text_box((yyvsp[(2) - (2)].str)); }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 200 "eqn.y"
    { (yyval.b) = new half_space_box; }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 202 "eqn.y"
    { (yyval.b) = new space_box; }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 204 "eqn.y"
    { (yyval.b) = new tab_box; }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 206 "eqn.y"
    { (yyval.b) = (yyvsp[(2) - (3)].b); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 208 "eqn.y"
    { (yyvsp[(2) - (2)].pb)->set_alignment(CENTER_ALIGN); (yyval.b) = (yyvsp[(2) - (2)].pb); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 210 "eqn.y"
    { (yyvsp[(2) - (2)].pb)->set_alignment(LEFT_ALIGN); (yyval.b) = (yyvsp[(2) - (2)].pb); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 212 "eqn.y"
    { (yyvsp[(2) - (2)].pb)->set_alignment(RIGHT_ALIGN); (yyval.b) = (yyvsp[(2) - (2)].pb); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 214 "eqn.y"
    { (yyvsp[(2) - (2)].pb)->set_alignment(CENTER_ALIGN); (yyval.b) = (yyvsp[(2) - (2)].pb); }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 216 "eqn.y"
    { (yyval.b) = (yyvsp[(3) - (4)].mb); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 218 "eqn.y"
    { (yyval.b) = make_delim_box((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].b), (yyvsp[(5) - (5)].str)); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 220 "eqn.y"
    { (yyval.b) = make_delim_box((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].b), 0); }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 222 "eqn.y"
    { (yyval.b) = make_overline_box((yyvsp[(1) - (2)].b)); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 224 "eqn.y"
    { (yyval.b) = make_underline_box((yyvsp[(1) - (2)].b)); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 226 "eqn.y"
    { (yyval.b) = make_prime_box((yyvsp[(1) - (2)].b)); }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 228 "eqn.y"
    { (yyval.b) = make_accent_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b)); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 230 "eqn.y"
    { (yyval.b) = make_uaccent_box((yyvsp[(1) - (3)].b), (yyvsp[(3) - (3)].b)); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 232 "eqn.y"
    { (yyval.b) = new font_box(strsave(get_grfont()), (yyvsp[(2) - (2)].b)); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 234 "eqn.y"
    { (yyval.b) = new font_box(strsave(get_gbfont()), (yyvsp[(2) - (2)].b)); }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 236 "eqn.y"
    { (yyval.b) = new font_box(strsave(get_gfont()), (yyvsp[(2) - (2)].b)); }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 238 "eqn.y"
    { (yyval.b) = new fat_box((yyvsp[(2) - (2)].b)); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 240 "eqn.y"
    { (yyval.b) = new font_box((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].b)); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 242 "eqn.y"
    { (yyval.b) = new size_box((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].b)); }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 244 "eqn.y"
    { (yyval.b) = new hmotion_box((yyvsp[(2) - (3)].n), (yyvsp[(3) - (3)].b)); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 246 "eqn.y"
    { (yyval.b) = new hmotion_box(-(yyvsp[(2) - (3)].n), (yyvsp[(3) - (3)].b)); }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 248 "eqn.y"
    { (yyval.b) = new vmotion_box((yyvsp[(2) - (3)].n), (yyvsp[(3) - (3)].b)); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 250 "eqn.y"
    { (yyval.b) = new vmotion_box(-(yyvsp[(2) - (3)].n), (yyvsp[(3) - (3)].b)); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 252 "eqn.y"
    { (yyvsp[(3) - (3)].b)->set_spacing_type((yyvsp[(2) - (3)].str)); (yyval.b) = (yyvsp[(3) - (3)].b); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 254 "eqn.y"
    { (yyval.b) = new vcenter_box((yyvsp[(2) - (2)].b)); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 256 "eqn.y"
    { (yyval.b) = make_special_box((yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].b)); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 261 "eqn.y"
    {
		  int n;
		  if (sscanf((yyvsp[(1) - (1)].str), "%d", &n) == 1)
		    (yyval.n) = n;
		  a_delete (yyvsp[(1) - (1)].str);
		}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 271 "eqn.y"
    { (yyval.pb) = new pile_box((yyvsp[(1) - (1)].b)); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 273 "eqn.y"
    { (yyvsp[(1) - (3)].pb)->append((yyvsp[(3) - (3)].b)); (yyval.pb) = (yyvsp[(1) - (3)].pb); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 278 "eqn.y"
    { (yyval.pb) = (yyvsp[(2) - (3)].pb); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 280 "eqn.y"
    { (yyvsp[(3) - (4)].pb)->set_space((yyvsp[(1) - (4)].n)); (yyval.pb) = (yyvsp[(3) - (4)].pb); }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 285 "eqn.y"
    { (yyval.mb) = new matrix_box((yyvsp[(1) - (1)].col)); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 287 "eqn.y"
    { (yyvsp[(1) - (2)].mb)->append((yyvsp[(2) - (2)].col)); (yyval.mb) = (yyvsp[(1) - (2)].mb); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 292 "eqn.y"
    { (yyval.col) = new column((yyvsp[(1) - (1)].b)); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 294 "eqn.y"
    { (yyvsp[(1) - (3)].col)->append((yyvsp[(3) - (3)].b)); (yyval.col) = (yyvsp[(1) - (3)].col); }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 299 "eqn.y"
    { (yyval.col) = (yyvsp[(2) - (3)].col); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 301 "eqn.y"
    { (yyvsp[(3) - (4)].col)->set_space((yyvsp[(1) - (4)].n)); (yyval.col) = (yyvsp[(3) - (4)].col); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 306 "eqn.y"
    { (yyvsp[(2) - (2)].col)->set_alignment(CENTER_ALIGN); (yyval.col) = (yyvsp[(2) - (2)].col); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 308 "eqn.y"
    { (yyvsp[(2) - (2)].col)->set_alignment(LEFT_ALIGN); (yyval.col) = (yyvsp[(2) - (2)].col); }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 310 "eqn.y"
    { (yyvsp[(2) - (2)].col)->set_alignment(RIGHT_ALIGN); (yyval.col) = (yyvsp[(2) - (2)].col); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 312 "eqn.y"
    { (yyvsp[(2) - (2)].col)->set_alignment(CENTER_ALIGN); (yyval.col) = (yyvsp[(2) - (2)].col); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 316 "eqn.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 318 "eqn.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 323 "eqn.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 325 "eqn.y"
    { (yyval.str) = strsave("{"); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 327 "eqn.y"
    { (yyval.str) = strsave("}"); }
    break;



/* Line 1455 of yacc.c  */
#line 2156 "eqn.cpp"
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
#line 330 "eqn.y"


