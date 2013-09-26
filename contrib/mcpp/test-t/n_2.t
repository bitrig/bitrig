/* n_2.t:   Line splicing by <backslash><newline> sequence. */

/* 2.1: In a #define directive line, between the parameter list and the
        replacement text.   */
/*  ab + cd + ef;   */
#define FUNC( a, b, c)  \
        a + b + c
    FUNC( ab, cd, ef);

/* 2.2: In a #define directive line, among the parameter list and among the
        replacement text.   */
/*  ab + cd + ef;   */
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

/* 2.5: <backslash><newline> by trigraph.   */
/*  abcde   */
    abc??/
de

