/* n_30.t:  Macro call. */
/*  Note:   Comma separate the arguments of function-like macro call,
        but comma between matching inner parenthesis doesn't.  This feature
        is tested on so many places in this suite especially on *.c samples
        which use assert() macro, that no separete item to test this feature
        is provided.    */

/* 30.1:    A macro call may cross the lines.   */
#define FUNC( a, b, c)      a + b + c
/*  a + b + c;  */
    FUNC
    (
        a,
        b,
        c
    )
    ;

