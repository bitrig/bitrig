/* n_7.t:   #line directive.    */

/* 7.1: Line number and filename.   */
/*  1234; "cpp";    */
#line   1234    "cpp"
    __LINE__; __FILE__;

/* 7.2: Filename argument is optional.  */
/*  2345; "cpp";    */
#line   2345
    __LINE__; __FILE__;

/* 7.3: Argument with macro.    */
/*  3456; "n_7.t";  */
#define LINE_AND_FILENAME   3456 "n_7.t"
#line   LINE_AND_FILENAME
    __LINE__; __FILE__;

