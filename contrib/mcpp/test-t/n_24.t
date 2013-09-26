/* n_24.t:  # operator in macro definition. */

#define str( a)     # a

/* 24.1:    White spaces should not be inserted (silly specification).  */
/*  "a+b";  */
    str( a+b);

/* 24.2:    White spaces between tokens of operand are converted to one space.
 */
/*  "ab + cd";  */
    str(    ab  /* comment */   +
        cd  );

/* 24.3:    \ is inserted before \ and " in or surrounding literals and no
        other character is inserted to anywhere.    */
/*  "'\"' + \"' \\\"\"";    */
    str( '"' + "' \"");

/* 24.4:    Line splicing by <backslash><newline> is done prior to token
        parsing.   */
/*  "\"abc\"";  */
    str( "ab\
c");

/* 24.5:    Token separator inserted by macro expansion should be removed.
        (Meanwhile, tokens should not be merged.  See 21.2.)    */
#define xstr( a)    str( a)
#define f(a)        a
/*  "x-y";  */
    xstr( x-f(y));

