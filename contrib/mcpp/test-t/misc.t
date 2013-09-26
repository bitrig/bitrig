/*
 *          M I S C . T
 *  1998/08     Made public.                                    kmatsui
 *  2003/11     Slightly revised.                               kmatsui
 *  2004/11     Split samples for "Reiser cpp" as "trad.t".
 *              Removed duplicate samples with other testcases.
 *                                                              kmatsui
 *
 *  Miscellaneous texts for test of preprocessor.
 *
 *  PART 1, 2   are excerpts from
 *      ISO C 6.8.3 "Examples"
 *  Some other examples are excerpted from :
 *      P.J.Plauger "The Standard C Library",
 *      GCC "cpp.info"
 */

/*
 *  PART 1
 *  Weird tests of macro rescan.
 */

#define x       3
#define f(a)    f(x * (a))
#undef  x

#define x       2
#define g       f
#define z       z[0]
#define h       g(~
#define m(a)    a(w)
#define w       0,1
#define t(a)    a

rescan()
{
/* f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);
 *
 * should expand to:
 *
 *      f(2 * (y+1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);
 */

f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);

/* g(x+(3,4)-w) | h 5) & m
 *     (f)^m(m);
 *
 * should result in:
 *
 *      f(2 * (2+(3,4)-0,1)) | f(2 * (~ 5)) & f(2 * (0,1))^m(0,1);
 */

g(x+(3,4)-w) | h 5) & m
    (f)^m(m);
}

/*
 *  PART 2
 *  Tests of creating string literal and concatenation of tokens.
 */

#define str(s)      # s
#define xstr(s)     str(s)
#define debug(s,t)  printf("x" # s "= %d, x" # t "= %s", \
                    x ## s, x ## t)
#define INCFILE(n)  vers ## n   /* comment  */
#define glue(a, b)  a ## b
#define xglue(a, b) glue(a, b)
#define HIGHLOW     "hello"
#define LOW         LOW ", world"

stringize()
{
/* debug(1, 2);
 *
 * should result in:
 *
 *      printf("x" "1" "= %d, x" "2" "= %s", x1, x2);
 *
 * after concatenation of the character string literals:
 *
 *      printf("x1= %d, x2= %s", x1, x2);
 */

debug(1, 2);

/* fputs(str(strncmp("abc\0d", "abc", '\4')
 *     == 0) str(: @\n), s);
 *
 * should result in:
 *
 *      fputs("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0" ": @\n", s);
 *
 * after concatenation of the character string literals (exactly, the each
 * escape sequences are converted to a corresponding character prior to the
 * concatenation):
 *
 *      fputs("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0: @\n", s);
 */

fputs(str(strncmp("abc\0d", "abc", '\4')    /* this goes away   */
    == 0) str(: @\n), s);

/* glue(HIGH, LOW);
 *
 * should result in:
 *
 *      "hello";
 */

glue(HIGH, LOW);

/* xglue(HIGH, LOW)
 *
 * should result in:
 *
 *      "hello" ", world"
 *
 * after concatenation of the character string literals:
 *
 *      "hello, world"
 */

xglue(HIGH, LOW)
}

#undef  f
#undef  g
#undef  h
#undef  m
#undef  t
#undef  w
#undef  x
#undef  z

/* #include xstr(INCFILE(2).h)
 *
 * should expand to (after macro replacement, before file access):
 *
 *      #include "vers2.h"
 *
 * "vers2.h" includes <stdio.h> and <limits.h> for the later tests.
 */

#include xstr(INCFILE(2).h)

/*
 *  PART 3
 *  More tests of token concatenation, taken from
 *  Kernighan & Ritchie "The C Programming Language", 2nd. Ed., A12.3.
 */

concat()
{
/* This is expanded to:
 *      abc;
 */
    xglue( xglue( a, b), c);

/* This is also expanded to:
 *      abc;
 */
    xglue( glue( a, b), c);

/* Results of these macro calls are undefined, once expanded to:
 *      glue( a, b)c;
 *      xglue( a, b)c;
 * ')c' is not a valid preprocessing token.
 */
    glue( glue( a, b), c);
    glue( xglue( a, b), c);
}

/*
 *  PART 4
 *  More samples of token concatenation.
 */

#define COMMAND( name)      { # name, name ## _command }

    struct command commands[] = {
/*      { "quit", quit_command },   */
        COMMAND (quit),
/*      { "help", help_command },   */
        COMMAND (help),
    };

#define maxof( type)    maxof ## type
#define DefMax( type)                   \
    type maxof( type)( type a, type b)  \
    {                                   \
        return (( a > b) ? a : b);      \
    }

/*      int maxofint( int a, int b) { return (( a > b) ? a : b); }  */
    DefMax( int)
/*      double maxofdouble( double a, double b)
 *      { return (( a > b) ? a : b); }
 */
    DefMax( double)

/*
 *  PART 5
 *  Test of stringization.
 */

line()
{
/*  211; "211"; "__LINE__"; */
    __LINE__;  xstr( __LINE__);  str( __LINE__);
}

/*
 *  PART 6
 *  Tests of handling escape sequences.
 *  Tests of concatenation of string literals.
 */

escape()
{
#undef  NDEBUG
#include    <assert.h>

/* In ASCII character set   */

    assert( '\a' == 7);

/* After string concatenation   :
 * results somehow as this:
 *      ((strcmp( "132", "132") == 0) ? (void) 0 :
 *          _assert( "strcmp( \"\\x31\" \"32\", \"132\") == 0",
 *          "misc.t",270));
 *      ((strcmp( "132", "132") == 0) ? (void) 0 :
 *          _assert( "strcmp( \"\\61\" \"32\", \"132\") == 0",
 *          "misc.t",271));
 */
    assert( strcmp( "\x31" "32", "132") == 0);
    assert( strcmp( "\61" "32", "132") == 0);

/*
 * In the messy encoding of KANJI called shift-JIS, "表" is encoded as
 * "\x95\x5C", the latter byte is not a beginning of an escape sequence.
 */
    assert( strcmp( "表" "作成", "表作成") == 0);
}

/*
 *  PART 7
 *  Tests of evaluating constant expression in long, unsigned long.
 *  Tests of <limits.h>, <assert.h>.
 */

#include    <limits.h>

#if     INT_MAX < INT_MIN || UINT_MAX <= INT_MAX
#   error  bad int properties
#endif
#if     LONG_MAX < 2147483647 || -2147483647 < LONG_MIN || LONG_MAX < INT_MAX
#   error  bad long properties
#endif

#define NDEBUG
#include    <assert.h>
assert_long()
{
/* This macro is ineffective now that NDEBUG is defined.    */
    assert( LONG_MAX >= 2147483647
        && -2147483647 >= LONG_MIN
        && LONG_MAX >= INT_MAX);
}

ulong()
{
/* For the compiler which has unsigned long data type   */
#if     defined ULONG_MAX && (ULONG_MAX < 4294967295    \
        || ULONG_MAX / 2 < LONG_MAX || ULONG_MAX < UINT_MAX)
#   error   bad unsigned long properties
#endif
}

internal()
{
/* The following 2 samlpes overflow on two's complement representation. */
#if     -LONG_MIN == LONG_MAX
    This is not a "two's complement" representation.
#endif
#if     LONG_MIN * -1 == LONG_MAX
    This is not a "two's complement" representation.
#endif

/* On two's complement representation this expression evaluate to false.    */
#if     defined (ULONG_MAX) && ULONG_MAX / 2 > LONG_MIN
    This is not "two's complement" representation or mis-implementation.
#endif

/* Right bit-shift of negative number is implementation-defined.    */
#if     -2 >> 1 == -1
    Arithmetic right shift of negative number.
#elif   -2 >> 1 == LONG_MAX
    Logical shift of negative number of "two's complement" representation.
#else
    Maybe logical shift of other than "two's complement" representation.
#endif

/* Dividing or modulo containing negative operand is not portable.  */
#if     -7 / 3 != -2
    Not algebraic dividing of negative number.
#endif
#if     -7 % 3 != -1
    What ?
#endif
}

/*
 *  PART 8
 *  Extended or obsolete facilities.
 */

non_standard()
{
#if     defined __cplusplus || __STDC_VERSION__ == 199901L
    // Comment in C++ style, terminating with '\x95\x5C' encoded in shift-JIS 表
    "__cplusplus" is defined or "__STDC_VERSION__" equals 199901L.
#endif

/* sizeof in #if expression is disallowed in Standard C */
#if     sizeof (long) < sizeof (int)
#   error   bad long properties
#endif
#if     sizeof (char *) == 2 && sizeof (int (*)()) == 2
    puts( "This is a small memory model.");
#endif

#if     __MCPP > 1
/* Trace the process of macro expansion */
#ifdef  __STDC__
#pragma MCPP debug memory       /* list heap memory */
#pragma MCPP debug token        /* trace token  */
#else
#debug  memory
#debug  token
#endif
/*  glue ( a, b)c;  */
    glue( glue( a, b), c);
#ifdef  __STDC__
#pragma MCPP end_debug token    /* no debug     */
#else
#end_debug token
#endif
#endif  /* __MCPP  */

#define TRUE    1
#define FALSE   0
#ifndef __STDC__
#ifdef  __MCPP
#assert TRUE != 0 && FALSE == 0
#endif
#endif
}

put_long()
{
    char * line = "line";
#ifndef __STDC__
#ifdef  __MWC09__
/* Assemly source for Microware-C/09    */
/* fputs( line, stdout);    */
#asm
    leax    _iob+13,y       /* stdout   */
    pshs    x
    ldd     2,s             /* line     */
    pshs    d
    lbsr    fputs
    leas    4,s
#endasm
#endif
#endif
}

/*
 *  PART 9
 *  Ambiguous macros and others.
 */
wide_and_nonwide()
{
/* Wide-character string literal following string literal and vice versa.   */
    "string" L"漢字";
    L"漢字" "string";
}

ambiguous()
{
/* Result of "f(2)(9)" is left intentionally ambiguous by Standard (ANSI C
    "Rationale", 3.8.3.4).  */
/*      2*f(9);  or  2*9*g; */
#define f(a)    a*g
#define g(a)    f(a)
    f(2)(9);
}

