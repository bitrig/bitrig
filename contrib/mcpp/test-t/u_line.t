/* u_line.t:    Undefined behaviors on out-of-range #line number.   */

/*  C99: Line number argument of #line directive should be in range of
    [1..2147483647] */

#line   2147483647  /* valid here   */
/* line 2147483647  */
/* line 2147483648 ? : out of range */
    __LINE__;   /* 2147483649 ? or -2147483647 ?,
                maybe warned as an out-of-range */
#line   0
#line   2147483648
