/*
 *      n_post.t
 *
 * 1998/08      made public                                     kmatsui
 * 2002/08      revised not to conflict with C99 Standard       kmatsui
 *
 *   Samples to test "POST_STANDARD" mode of C preprocessing.
 *   POST_STANDARD cpp must process these samples as shown in each comments.
 *   POST_STANDARD cpp must process also #error directive properly, which is
 * not included here because the directive might cause translator to
 * terminate.
 */


#define ZERO_TOKEN
#define TWO_ARGS        a,b
#define MACRO_0         0
#define MACRO_1         1
#define sub( x, y)      (x - y)
#define str( a)         # a
#define glue( a, b)     a ## b
#define xglue( a, b)    glue( a, b)


/* n_2.t:   Line splicing by <backslash><newline> sequence. */

/* 2.1: In a #define directive line, between the parameter list and the
        replacement text.   */
/*  ab + cd + ef ;  */
#define FUNC( a, b, c)  \
        a + b + c
    FUNC( ab, cd, ef);

/* 2.2: In a #define directive line, among the parameter list and among the
        replacement text.   */
/*  ab + cd + ef ;  */
#undef  FUNC
#define FUNC( a, b  \
        , c)        \
        a + b       \
        + c
    FUNC( ab, cd, ef);

/* 2.3: In a string literal.    */
/*  "abcde" */
    "abc\
de"

/* 2.4: <backslash><newline> in midst of an identifier. */
/*  abcde   */
    abc\
de



/* n_3.t:   Handling of comment.    */

/* 3.1: A comment is converted to one space.    */
/*  abc de  */
    abc/* comment */de

/* 3.2: // is not a comment of C.   */
#if 0   /* This feature is obsolete now.  */
/*  / / is not a comment of C   */
    // is not a comment of C
#endif

/* 3.3: Comment is parsed prior to the parsing of preprocessing directive.  */
/*  abcd    */
#if     0
    "nonsence"; /*
#else
    still in
    comment     */
#else
#define MACRO_abcd  /*
    in comment
    */  abcd
#endif
    MACRO_abcd


/* n_4.t:   Special tokens. */

/* 4.1: Digraph spellings in directive line.    */
/*  "abc" ; */
%: define  stringize( a)    %: a

    stringize( abc);

/* 4.2: Digraph spellings are converted in translation phase 1. */
/*  "[" ;   */
    stringize( <:);


/* n_5.t:   Spaces or tabs are allowed at any place in pp-directive line,
        including between the top of a pp-directive line and '#', and between
        the '#' and the directive. */

/* 5.1: */
/*  |**|[TAB]# |**|[TAB]define |**| MACRO_abcde[TAB]|**| abcde |**| */
/**/    # /**/  define /**/ MACRO_abcde /**/ abcde /**/
/*  abcde   */
    MACRO_abcde


/* n_6.t:   #include directive. */

/* 6.1: Header-name quoted by " and " as well as by < and > can include
        standard headers.   */
/* Note: Standard headers can be included any times.    */
#include    "ctype.h"
#include    <ctype.h>       /* An obsolescent feature.  */

/* 6.2: Macro is allowed in #include line.  */
#define HEADER  "header.h"
/* Before file inclusion:   #include "header.h" */
#include    HEADER
/*  abc */
    MACRO_abc

/* 6.3: With macro nonsence but legal.  */
#undef  MACRO_abc
#include    ZERO_TOKEN HEADER ZERO_TOKEN
/*  abc */
    MACRO_abc


/* n_7.t:   #line directive.    */

/* 7.1: Line number and filename.   */
/*  1234 ; "cpp" ;  */
#line   1234    "cpp"
    __LINE__; __FILE__;

/* 7.2: Filename argument is optional.  */
/*  2345 ; "cpp" ;  */
#line   2345
    __LINE__; __FILE__;

/* 7.3: Argument with macro.    */
/*  3456 ; "n_7.t" ;    */
#define LINE_AND_FILENAME   3456 "n_7.t"
#line   LINE_AND_FILENAME
    __LINE__; __FILE__;

/* Restore to correct line number and filename. */
#line   149 "n_post.t"


/* n_9.t:   #pragma directive.  */

/* 9.1: Any #pragma directive should be processed or ignored, should not
        be diagnosed as an error.   */
#pragma once
#pragma who knows ?


/* n_10.t:  #if, #elif, #else and #endif pp-directive.  */

/* 10.1:    */
/* Note: an undefined identifier in #if expression is replaced to 0.    */
/*  1 ; */
#if     a
    a;
#elif MACRO_0
    MACRO_0;
#elif   MACRO_1         /* Valid block  */
    MACRO_1;
#else
    0;
#endif

/* 10.2:    Comments must be processed even if in skipped #if block.    */
/* At least tokenization of string literal and character constant is necessary
        to process comments, e.g. /* is not a comment mark in string literal.
 */
#ifdef  UNDEFINED
    /* Comment  */
    "in literal /* is not a comment"
#endif


/* n_11.t:  Operator "defined" in #if or #elif directive.   */

/* 11.1:    */
#undef  MACRO_abc
#define MACRO_abc   abc
/*  abc ;   */
/*  abc ;   */
#if     defined a
    a;
#else
    MACRO_abc;
#endif
#if     defined (MACRO_abc)
    MACRO_abc;
#else
    0;
#endif

/* 11.2:    "defined" is an unary operator whose result is 1 or 0.  */
#if     defined MACRO_0 * 3 != 3
    Bad handling of "defined" operator.
#endif
#if     (!defined ZERO_TOKEN != 0) || (-defined ZERO_TOKEN != -1)
    Bad grouping of "defined", !, - operator.
#endif


/* n_12.t:  Integer preprocessing number token and type of #if expression.  */

#include    "limits.h"

/* 12.1:    Type long.  */
#if     LONG_MAX <= LONG_MIN
    Bad evaluation of long.
#endif
#if     LONG_MAX <= 1073741823  /* 0x3FFFFFFF   */
    Bad evaluation of long.
#endif

/* 12.2:    Type unsigned long. */
#if     ULONG_MAX / 2 < LONG_MAX
    Bad evaluation of unsigned long.
#endif

/* 12.3:    Octal number.   */
#if     0177777 != 65535
    Bad evaluation of octal number.
#endif

/* 12.4:    Hexadecimal number. */
#if     0Xffff != 65535 || 0xFfFf != 65535
    Bad evaluation of hexadecimal number.
#endif

/* 12.5:    Suffix 'L' or 'l'.  */
#if     0L != 0 || 0l != 0
    Bad evaluation of 'L' suffix.
#endif

/* 12.6:    Suffix 'U' or 'u'.  */
#if     1U != 1 || 1u != 1
    Bad evaluation of 'U' suffix.
#endif


/* n_13.t:  Valid operators in #if expression.  */

/* Valid operators are (precedence in this order) :
    defined, (unary)+, (unary)-, ~, !,
    *, /, %,
    +, -,
    <<, >>,
    <, >, <=, >=,
    ==, !=,
    &,
    ^,
    |,
    &&,
    ||,
    ? :
 */

/* 13.1:    Bit shift.  */
#if     1 << 2 != 4 || 8 >> 1 != 4
    Bad arithmetic of <<, >> operators.
#endif

/* 13.2:    Bitwise operators.  */
#if     (3 ^ 5) != 6 || (3 | 5) != 7 || (3 & 5) != 1
    Bad arithmetic of ^, |, & operators.
#endif

/* 13.3:    Result of ||, && operators is either of 1 or 0. */
#if     (2 || 3) != 1 || (2 && 3) != 1 || (0 || 4) != 1 || (0 && 5) != 0
    Bad arithmetic of ||, && operators.
#endif

/* 13.4:    ?, : operator.  */
#if     (0 ? 1 : 2) != 2
    Bad arithmetic of ?: operator.
#endif


/* n_13_5.t:    Arithmetic conversion in #if expressions.   */

/* 13.5:    The usual arithmetic conversion is not performed on bit shift.  */
#if     -1 << 3U > 0
    Bad conversion of bit shift operands.
#endif

/* 13.6:    Usual arithmetic conversions.   */
#if     -1 <= 0U        /* -1 is converted to unsigned long.    */
    Bad arithmetic conversion.
#endif

#if     -1 * 1U <= 0
    Bad arithmetic conversion.
#endif

/* Second and third operands of conditional operator are converted to the
        same type, thus -1 is converted to unsigned long.    */
#if     (1 ? -1 : 0U) <= 0
    Bad arithmetic conversion.
#endif


/* n_13_7.t:    Short-circuit evaluation of #if expression. */

/* 13.7:    10/0 or 10/MACRO_0 are never evaluated, "divide by zero" error
        cannot occur.   */
/*  Valid block */
#if     0 && 10 / 0
    Block to be skipped
#endif
#if     not_defined && 10 / not_defined
    Block to be skipped
#endif
#if     MACRO_0 && 10 / MACRO_0 > 1
    Block to be skipped
#endif
#if     MACRO_0 ? 10 / MACRO_0 : 0
    Block to be skipped
#endif
#if     MACRO_0 == 0 || 10 / MACRO_0 > 1
    Valid block
#else
    Block to be skipped
#endif


/* n_13_8.t:    Grouping of sub-expressions in #if expression.  */

/* 13.8:    Unary operators are grouped from right to left. */
#if     (- -1 != 1) || (!!9 != 1) || (-!+!9 != -1) || (~~1 != 1)
    Bad grouping of -, +, !, ~ in #if expression.
#endif

/* 13.9:    ?: operators are grouped from right to left.    */
#if     (1 ? 2 ? 3 ? 3 : 2 : 1 : 0) != 3
    Bad grouping of ? : in #if expression.
#endif

/* 13.10:   Other operators are grouped from left to right. */
#if     (15 >> 2 >> 1 != 1) || (3 << 2 << 1 != 24)
    Bad grouping of >>, << in #if expression.
#endif

/* 13.11:   Test of precedence. */
#if     3*10/2 >> !0*2 >> !+!-9 != 1
    Bad grouping of -, +, !, *, /, >> in #if expression.
#endif

/* 13.12:   Overall test.  Grouped as:
        ((((((+1 - -1 - ~~1 - -!0) & 6) | ((8 % 9) ^ (-2 * -2))) >> 1) == 7)
        ? 7 : 0) != 7
    evaluated to FALSE.
 */
#if     (((+1- -1-~~1- -!0&6|8%9^-2*-2)>>1)==7?7:0)!=7
    Bad arithmetic of #if expression.
#endif


/* n_13_13.t:   #if expression with macros. */

#define and             &&
#define or              ||
#define not_eq          !=
#define bitor           |

/* 13.13:   With macros expanding to operators. */
/*  Valid block */
#if     (1 bitor 2) == 3 and 4 not_eq 5 or 0
    /* #if (1 | 2) == 3 && 4 != 5 || 0  */
    Valid block
#else
    Block to be skipped
#endif

/* 13.14:   With macros expanding to 0 token, nonsence but legal expression.*/
/*  Valid block */
#if     ZERO_TOKEN MACRO_1 ZERO_TOKEN > ZERO_TOKEN MACRO_0 ZERO_TOKEN
    /* #if 1 > 0    */
    Valid block
#else
    Block to be skipped
#endif


/* n_15.t:  #ifdef, #ifndef directives. */

/* 15.1:    #ifdef directive.   */
/*  Valid block */
#ifdef  MACRO_1
    Valid block
#else
    Block to be skipped
#endif

/* 15.2:    #ifndef directive.  */
/*  Valid block */
#ifndef MACRO_1
    Block to be skipped
#else
    Valid block
#endif


/* n_18.t:  #define directive.  */

/* Excerpts from ISO C 6.8.3 "Examples".    */
#define OBJ_LIKE        (1-1)
#define FTN_LIKE(a)     ( a )

/* 18.1:    Definition of an object-like macro. */
/*  ( 1 - 1 ) ; */
    OBJ_LIKE;

/* 18.2:    Definition of a function-like macro.    */
/*  ( c ) ; */
    FTN_LIKE( c);

/* 18.3:    Spelling in string identical to parameter is not a parameter.   */
/*  "n1:n2" ;   */
#define STR( n1, n2)    "n1:n2"
    STR( 1, 2);


/* n_19.t:  Valid re-definitions of macros. */

/* 19.1:    */
#define OBJ_LIKE    /* white space */  (1-1) /* other */

/* 19.2:    */
#define FTN_LIKE( a     )(  /* note the white space */  \
                        a  /* other stuff on this line
                           */ )

/* 19.4:    */
#define OBJ_LIKE        (1 - 1) /* different white space        */

/* 19.6:    */
#define FTN_LIKE(b)     ( b )   /* different parameter spelling */

/*  ( c ) ; */
    FTN_LIKE( c);


/* n_20.t:  Definition of macro lexically identical to keyword. */

/* 20.1:    */
/*  double fl ; */
#define float   double
    float   fl;


/* n_21.t:  Tokenization (No preprocessing tokens are merged implicitly).   */

/* 21.1:    */
/*  - - - a ;   */
#define MINUS   -
    -MINUS-a;


/* n_22.t:  Tokenization of preprocessing number.   */

#define EXP         1

/* 22.1:    12E+EXP is a preprocessing number, EXP is not expanded. */
/*  12E+EXP ;   */
    12E+EXP;

/* 22.2:    .2e-EXP is also a preprocessing number. */
/*  .2e-EXP ;   */
    .2e-EXP;

/* 22.3:    + or - is allowed only following E or e, 12+EXP is not a
        preprocessing number.   */
/* Three tokens: 12 + 1 ;   */
    12+EXP;


/* n_23.t:  ## operator in macro definition.    */

/* 23.1:    */
/*  xy ;    */
    glue( x, y);

/* 23.2:    Generate a preprocessing number.    */
/*  .12e+2 ;    */
#undef  EXP
#define EXP     2
    xglue( .12e+, EXP);


/* n_24.t:  # operator in macro definition. */

/* 24.1:    */
/*  "a + b" ;   */
    str( a+b);

/* 24.2:    White spaces between tokens of operand are converted to one space.
 */
/*  "ab + cd" ; */
    str(    ab  /* comment */   +
        cd  );

/* 24.3:    \ is inserted before \ and " in or surrounding literals and no
        other character is inserted to anywhere.    */
/*  "'\"' + \"' \\\"\"" ;   */
    str( '"' + "' \"");

/* 24.4:    Line splicing by <backslash><newline> is done prior to token
        parsing.    */
/*  "\"abc\"" ; */
    str( "ab\
c");


/* n_25.t:  Macro arguments are pre-expanded (unless the argument is an
        operand of # or ## operator) separately, that is, macro-replaced
        completely prior to rescanning. */

/* 25.1:    "TWO_ARGS" is read as one argument to "sub", then expanded to
        "a , b", then "x" is substituted by "a , b".    */
/*  ( a , b - 1 ) ; */
    sub( TWO_ARGS, 1);

/* 25.2:    An argument pre-expanded to 0-token.    */
/*  (  - 1 ) ;  */
    sub( ZERO_TOKEN, 1);

/* 25.3:    "glue( a, b)" is pre-expanded.  */
/*  abc ;   */
    xglue( glue( a, b), c);

/* 25.4:    Operands of ## operator are not pre-expanded.   */
/*  MACRO_0MACRO_1 ;    */
    glue( MACRO_0, MACRO_1);

/* 25.5:    Operand of # operator is not pre-expanded.  */
/*  "ZERO_TOKEN" ;  */
    str( ZERO_TOKEN);


/* n_26.t:  The name once replaced is not furthur replaced. */

/* 26.1:    Directly recursive object-like macro definition.    */
/*  Z [ 0 ] ;   */
#define Z   Z[0]
    Z;

/* 26.2:    Intermediately recursive object-like macro definition.  */
/*  AB ;    */
#define AB  BA
#define BA  AB
    AB;

/* 26.3:    Directly recursive function-like macro definition.  */
/*  x + f ( x ) ;   */
#define f(a)    a + f(a)
    f( x);

/* 26.4:    Intermediately recursive function-like macro definition.    */
/*  x + x + g ( x ) ;   */
#define g(a)    a + h( a)
#define h(a)    a + g( a)
    g( x);

/* 26.5:    Rescanning encounters the non-replaced macro name.  */
/*  Z [ 0 ] + f ( Z [ 0 ] ) ;   */
    f( Z);


/* n_27.t:  Rescanning of a macro expand any macro call in the replacement
        text after substitution of parameters by pre-expanded-arguments.  This
        re-examination does not involve the succeding sequences from the
        source file.    */

/* 27.1:    Cascaded use of object-like macros. */
/*  1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 ; */
#define NEST8   NEST7 + 8
#define NEST7   NEST6 + 7
#define NEST6   NEST5 + 6
#define NEST5   NEST4 + 5
#define NEST4   NEST3 + 4
#define NEST3   NEST2 + 3
#define NEST2   NEST1 + 2
#define NEST1   1
    NEST8;

/* 27.2:    Cascaded use of function-like macros.   */
/*  ( 1 ) + ( 1 + 2 ) + 1 + 2 + 1 + 2 + 3 + 1 + 2 + 3 + 4 ; */
#define FUNC4( a, b)    FUNC3( a, b) + NEST4
#define FUNC3( a, b)    FUNC2( a, b) + NEST3
#define FUNC2( a, b)    FUNC1( a, b) + NEST2
#define FUNC1( a, b)    (a) + (b)
    FUNC4( NEST1, NEST2);

/* 27.3:    An identifier generated by ## operator is subject to expansion. */
/*  1 ; */
    glue( MACRO_, 1);

#define head            sub(
#define math( op, a, b) op( (a), (b))
#define SUB             sub

/* 27.4:    "sub" as an argument of math() is not pre-expanded, since '(' is
        missing.    */
/*  ( ( a ) - ( b ) ) ; */
    math( sub, a, b);

/* 27.5:    Object-like macro expanded to name of a function-like macro.    */
/*  sub ( a , b ) ; */
    SUB( a, b);


/* n_28.t:  __FILE__, __LINE__, __DATE__, __TIME__, __STDC__ and
            __STDC_VERSION__ are predefined.    */

/* 28.1:    */
/*  "n_post.t" ;    */
    __FILE__;

/* 28.2:    */
/*  629 ;   */
    __LINE__;

/* 28.3:    */
/*  "Aug  1 2001";  */
    __DATE__;

/* 28.4:    */
/*  "21:42:22" ;    */
    __TIME__;

/* 28.5:    */
/*  1 ; */
    __STDC__;

/* 28.6:    */
/*  199409L ;   */
/* In C99, the value of this macro is 199901L   */
    __STDC_VERSION__;

/* 28.7:    __LINE__, __FILE__ in an included file. */
/*  3 ; "line.h" ;  */
#include    "line.h"


/* n_29.t:  #undef directive.   */

/* 29.1:    Undefined macro is not a macro. */
/*  DEFINED ;   */
#define DEFINED
#undef  DEFINED
    DEFINED;

/* 29.2:    Undefining undefined name is not an error.  */
#undef  UNDEFINED


/* n_30.t:  Macro call. */
/*  Note:   Comma separate the arguments of function-like macro call,
        but comma between matching inner parenthesis doesn't.  This feature
        is tested on so many places in this suite especially on *.c samples
        which use assert() macro, that no separete item to test this feature
        is provided.    */

/* 30.1:    A macro call may cross the lines.   */
#define FUNC( a, b, c)      a + b + c
/*  a + b + c ; */
    FUNC
    (
        a,
        b,
        c
    )
    ;


/* n_37.t:  Translation limits. */

/* 37.1:    Number of parameters in macro: at least 31. */
#define glue31(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E)   \
    a##b##c##d##e##f##g##h##i##j##k##l##m##n##o##p##q##r##s##t##u##v##w##x##y##z##A##B##C##D##E

/* 37.2:    Number of arguments of macro call: at least 31. */
/*  ABCDEFGHIJKLMNOPQRSTUVWXYZabcde ;   */
    glue31( A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R
            , S, T, U, V, W, X, Y, Z, a, b, c, d, e);

/* 37.3:    Significant initial characters in an internal identifier or a
        macro name: at least 31.  */
/*  ABCDEFGHIJKLMNOPQRSTUVWXYZabcd_ ;   */
    ABCDEFGHIJKLMNOPQRSTUVWXYZabcd_;

/* 37.4:    Nested conditional inclusion: at least 8 levels.    */
/*  nest = 8 ;  */
#ifdef  A
#else
#   ifdef   B
#   else
#       ifdef   C
#       else
#           ifdef   D
#           else
#               ifdef   E
#               else
#                   ifdef   F
#                   else
#                       ifdef   G
#                       else
#                           ifdef   H
#                           else
                                nest = 8;
#                           endif
#                       endif
#                   endif
#               endif
#           endif
#       endif
#   endif
#endif

/* 37.5:    Nested source file inclusion: at least 8 levels.    */
/*  nest = 1 ;  nest = 2 ;  nest = 3 ;  nest = 4 ;
    nest = 5 ;  nest = 6 ;  nest = 7 ;  nest = 8 ;  */
#define     X8
#include    "nest1.h"

/* 37.6:    Parenthesized expression: at least 32 levels.   */
/*  nest = 32 ; */
#if     0 + (1 - (2 + (3 - (4 + (5 - (6 + (7 - (8 + (9 - (10 + (11 - (12 +  \
        (13 - (14 + (15 - (16 + (17 - (18 + (19 - (20 + (21 - (22 + (23 -   \
        (24 + (25 - (26 + (27 - (28 + (29 - (30 + (31 - (32 + 0))))))))))   \
        )))))))))))))))))))))) == 0
    nest = 32;
#endif

/* 37.7:    Characters in a string (after concatenation): at least 509. */
"123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789\
012345678901234567"
        ;

/* 37.8:    Characters in a logical source line: at least 509.  */
    int a123456789012345678901234567890 = 123450;   \
    int b123456789012345678901234567890 = 123451;   \
    int c123456789012345678901234567890 = 123452;   \
    int d123456789012345678901234567890 = 123453;   \
    int e123456789012345678901234567890 = 123454;   \
    int f123456789012345678901234567890 = 123455;   \
    int A123456789012345678901234567890 = 123456;   \
    int B123456789012345678901234567890 = 123457;   \
    int C123456789012345678901234567890 = 123458;   \
    int D1234567890123456789012 = 123459;

/* 37.9:    Macro definitions: at least 1024.   */

#define X0400
#include    "m4095.h"
/*  0x0400; */
    BNJ;

