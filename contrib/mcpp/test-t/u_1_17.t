/* u_1_17.t:    Undefined behaviors on out-of-range #line number.   */

/* u.1.17:  Line number argument of #line directive should be in range of
        [1,32767].  */
/* In C99, range of line number is [1,2147483647]   */
#if __STDC_VERSION__ < 199901L
#line   32767   /* valid here   */
/* line 32767   */
/* line 32768 ? : out of range  */
    __LINE__;   /* 32769 ? or -32767 ?, maybe warned as an out-of-range */
#line   0
#line   32768
#else
#line   2147483647  /* valid here   */
/* line 2147483647  */
/* line 2147483648 ? : out of range */
    __LINE__;   /* 2147483649 ? or -2147483647 ?,
                maybe warned as an out-of-range */
#line   0
#line   2147483648
#endif

/* u.1.18:  Line number argument of #line directive should be written in
        decimal digits. */
#line   0x1000

