/* e_7_4.t:     #line error.    */

/* 7.4:     string literal in #line directive shall be a character string
        literal.    */

#line   123     L"wide"
/*  8; "e_7_4.t";   */
    __LINE__; __FILE__;

