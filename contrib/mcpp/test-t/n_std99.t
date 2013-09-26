/*
 *      n_std99.t
 *
 * 2002/08      kmatsui
 *
 *   Samples to test Standard C99 preprocessing.
 */

/* n_ucn1:  Universal-character-name    */ 

/* UCN in character constant    */

#if '\u5B57'
    UCN-16bits is implemented.
#endif

#if '\U00006F22'
    UCN-32bits is implemented.
#endif

/* UCN in string literal    */

    "abc\u6F22\u5B57xyz";   /* i.e. "abc´Á»úxyx";   */

/* UCN in identifier    */

#define macro\u5B57         9
#define macro\U00006F22     99

    macro\u5B57         /* 9    */
    macro\U00006F22     /* 99   */
    macro\U00006f22     /* 99   */

/* n_ucn2.t:    Universal-character-name    */
/* UCN in pp-number */

#define mkint( a)   a ## 1\u5B57

    int mkint( abc) = 0;  /* int abc1\u5B57 = 0;    */

/* n_dslcom.t:  // is a comment of C99. */
/*  a;  */ 
    a;  // is a comment of C99


/* n_ppnum.t: Preprocessing number token *p+*   */
/* Undefined on C90, because '+A' is not a valid pp-token.  */

#define A   3
#define glue( a, b) a ## b
/*  12p+A;  */
    glue( 12p+, A);


/* n_line.t:    line number argument of #line   */
/* C99: Range of line number in #line directive is [1..2147483647]  */

/*  2147483647; */
#line   2147483647
    __LINE__;
#line   62  /* Resume the real line number  */
    __LINE__;


/* n_llong.t:   long long in #if expression */

#if 12345678901234567890 < 13345678901234567890
    "long long #if expression is implemented."
#else
    "long long #if expression is not implemented."
#endif

#if 12345678901234567890ULL < 13345678901234567890ULL
    Valid block
#else
    Block to be skipped
#endif

#if (0x7FFFFFFFFFFFFFFFLL - 0x6FFFFFFFFFFFFFFFLL) >> 60 == 1
    Valid block
#else
    Block to be skipped
#endif


/* n_vargs.t:   Macro of variable arguments */
/* from C99 Standard 6.10.3 Examples        */
    #define debug( ...) fprintf( stderr, __VA_ARGS__)
    #define showlist( ...)  puts( #__VA_ARGS__)
    #define report( test, ...)  ((test) ? puts( #test)  \
            : printf( __VA_ARGS__))
    {
        /* fprintf( stderr, "Flag");    */
    debug( "Flag");
        /* fprintf( stderr, "X = %d\n", x);     */
    debug( "X = %d\n", x);
        /* puts( "The first, second, and third items.");   */
    showlist( The first, second, and third items.);
        /* ((x>y) ? puts( "x>y") : printf( "x is %d but y is %d", x, y));   */
    report( x>y, "x is %d but y is %d", x, y);
    }


/* n_pragma.t:  _Pragma() operator  */
/* based on the proposal to C99 by Bill Homer   */

#define Machine_B

#include "pragmas.h"

/* #pragma vfunction    */
Fast_call
void f(int n, double * a, double * b) {
/* #pragma ivdep    */
    Independent
    while(n-- > 0) {
        *a++ += *b++;
    }
}

#define f(N, A, B)  \
{   int n = (N), double * a = (A), double * b = (B); \
    Independent while(n-- > 0) { *a++ += *b++; } \
}

#define EXPRAG(x) _Pragma(#x)
#define PRAGMA(x) EXPRAG(x)
#define LIBFUNC xyz

int libfunc() { ... }

/* Direct the linker to define alternate entry points for libfunc. */
/* #pragma duplicate libfunc as (lib_func,xyz)  */
PRAGMA( duplicate libfunc as (lib_func,LIBFUNC) )


/* n_nularg.t:  Empty argument of macro call.   */

#define ARG( a)         # a
#define EMPTY
#define SHOWN( n)       printf( "%s : %d\n", # n, n)
#define SHOWS( s)       printf( "%s : %s\n", # s, ARG( s))
#define add( a, b)      (a + b)
#define sub( a, b)      (a - b)
#define math( op, a, b)     op( a, b)
#define APPEND( a, b)       a ## b

/*  printf( "%s : %d\n", "math( sub, , y)", ( - y));    */
    SHOWN( math( sub, , y));

/*  printf( "%s : %s\n", "EMPTY", "");  */
    SHOWS( EMPTY);

/*  printf( "%s : %s\n", "APPEND( CON, 1)", "CON1");    */
    SHOWS( APPEND( CON, 1));

/*  printf( "%s : %s\n", "APPEND( CON, )", "CON");  */
    SHOWS( APPEND( CON, ));

/*  printf( "%s : %s\n", "APPEND( , )", "");  */
    SHOWS( APPEND( , ));


/* n_stdmac.t:  C99 Standard pre-defined macros.    */

/*  199901L;    */
    __STDC_VERSION__;

/*  1; or 0;    */
    __STDC_HOSTED__;


/* n_tlimit.t:  Tests of translation limits.   */

/* 37.1L:    Number of parameters of macro definition: at least 127. */

#define glue127(    \
    a0, b0, c0, d0, e0, f0, g0, h0, i0, j0, k0, l0, m0, n0, o0, p0, \
    a1, b1, c1, d1, e1, f1, g1, h1, i1, j1, k1, l1, m1, n1, o1, p1, \
    a2, b2, c2, d2, e2, f2, g2, h2, i2, j2, k2, l2, m2, n2, o2, p2, \
    a3, b3, c3, d3, e3, f3, g3, h3, i3, j3, k3, l3, m3, n3, o3, p3, \
    a4, b4, c4, d4, e4, f4, g4, h4, i4, j4, k4, l4, m4, n4, o4, p4, \
    a5, b5, c5, d5, e5, f5, g5, h5, i5, j5, k5, l5, m5, n5, o5, p5, \
    a6, b6, c6, d6, e6, f6, g6, h6, i6, j6, k6, l6, m6, n6, o6, p6, \
    a7, b7, c7, d7, e7, f7, g7, h7, i7, j7, k7, l7, m7, n7, o7)     \
    a0 ## b0 ## c0 ## d0 ## e0 ## f0 ## g0 ## h0 ## \
    p0 ## p1 ## p2 ## p3 ## p4 ## p5 ## p6 ## o7

/* 37.2L:    Number of arguments of macro call: at least 127.    */

/*  A0B0C0D0E0F0G0H0P0P1P2P3P4P5P6O7;   */
    glue127(
    A0, B0, C0, D0, E0, F0, G0, H0, I0, J0, K0, L0, M0, N0, O0, P0,
    A1, B1, C1, D1, E1, F1, G1, H1, I1, J1, K1, L1, M1, N1, O1, P1,
    A2, B2, C2, D2, E2, F2, G2, H2, I2, J2, K2, L2, M2, N2, O2, P2,
    A3, B3, C3, D3, E3, F3, G3, H3, I3, J3, K3, L3, M3, N3, O3, P3,
    A4, B4, C4, D4, E4, F4, G4, H4, I4, J4, K4, L4, M4, N4, O4, P4,
    A5, B5, C5, D5, E5, F5, G5, H5, I5, J5, K5, L5, M5, N5, O5, P5,
    A6, B6, C6, D6, E6, F6, G6, H6, I6, J6, K6, L6, M6, N6, O6, P6,
    A7, B7, C7, D7, E7, F7, G7, H7, I7, J7, K7, L7, M7, N7, O7);

/* 37.3L:   Significant initial characters in an internal identifier or a
        macro name: at least 63 bytes.  */

    int
A23456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef = 63;

/*  37.4L:  Nested conditional inclusion: at least 63 level.    */

/* ifdef_nest = 0x3f;   */
#define     X3F
#include    "ifdef15.h"

/*  37.5L:  Nested source file inclusion: at least 15 level.    */

/* nest = 0x0f; */
#define     X0F
#include    "nest1.h"

/*  37.6L:  Parenthesized expression: at least 63 level.    */

/*  nest = 63;  */
#if \
        (0x00 + (0x01 - (0x02 + (0x03 - (0x04 + (0x05 - (0x06 + (0x07 - \
        (0x08 + (0x09 - (0x0A + (0x0B - (0x0C + (0x0D - (0x0E + (0x0F - \
        (0x10 + (0x11 - (0x12 + (0x13 - (0x14 + (0x15 - (0x16 + (0x17 - \
        (0x18 + (0x19 - (0x1A + (0x1B - (0x1C + (0x1D - (0x1E + (0x1F - \
        (0x20 + (0x21 - (0x22 + (0x23 - (0x24 + (0x25 - (0x26 + (0x27 - \
        (0x28 + (0x29 - (0x2A + (0x2B - (0x2C + (0x2D - (0x2E + (0x2F - \
        (0x30 + (0x31 - (0x32 + (0x33 - (0x34 + (0x35 - (0x36 + (0x37 - \
        (0x38 + (0x39 - (0x3A + (0x3B - (0x3C + (0x3D - 0x3E)           \
        )))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))   \
        == -1
    nest = 63;
#endif

/* 37.7L:   Characters in a string (after concatenation)
        : at least 4095 bytes.  */

/*  4095 bytes long.    */
    char    *string4093 =
"123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
2123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
3123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
4123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
5123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
6123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
7123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
8123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
9123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
a123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
b123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
c123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
d123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
e123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
f123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
A123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
2123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
3123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
4123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
5123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
6123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
7123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
8123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
9123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
a123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
b123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
c123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
d123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
e123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
f123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
B123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
2123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
3123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
4123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
5123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
6123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
7123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
8123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
9123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
a123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
b123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
c123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
d123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
e123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
f123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
C123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
2123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
3123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
4123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
5123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
6123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
7123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
8123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
9123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
a123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
b123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
c123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
d123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
e123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
f123456789abcdef0123456789abcdef0123456789abcdef0123456789abcd"
;

/* 37.8L:   Length of logical source line: at least 4095 bytes. */

#include    "long4095.h"

/* 37.9L:   Number of macro definitions: at least 4095. */

#undef  ARG
#include    "m4095.h"

/*  0x0fff; */
    GBM;

