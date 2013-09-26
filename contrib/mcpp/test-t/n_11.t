/* n_11.t:  Operator "defined" in #if or #elif directive.   */

#define MACRO_abc   abc
#define MACRO_0     0
#define ZERO_TOKEN

/* 11.1:    */
/*  abc;    */
/*  abc;    */
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

