/* n_25.t:  Macro arguments are pre-expanded (unless the argument is an
    operand of # or ## operator) separately, that is, are macro-replaced
    completely prior to rescanning. */

#define MACRO_0         0
#define MACRO_1         1
#define ZERO_TOKEN
#define TWO_ARGS        a,b
#define sub( x, y)      (x - y)
#define glue( a, b)     a ## b
#define xglue( a, b)    glue( a, b)
#define str( a)         # a

/* 25.1:    "TWO_ARGS" is read as one argument to "sub" then expanded to
        "a,b", then "x" is substituted by "a,b".    */
/*  (a,b - 1);  */
    sub( TWO_ARGS, 1);

/* 25.2:    An argument pre-expanded to 0-token.    */
/*  ( - 1); */
    sub( ZERO_TOKEN, 1);

/* 25.3:    "glue( a, b)" is pre-expanded.  */
/*  abc;    */
    xglue( glue( a, b), c);

/* 25.4:    Operands of ## operator are not pre-expanded.   */
/*  MACRO_0MACRO_1; */
    glue( MACRO_0, MACRO_1);

/* 25.5:    Operand of # operator is not pre-expanded.  */
/*  "ZERO_TOKEN";   */
#define ZERO_TOKEN
    str( ZERO_TOKEN);

