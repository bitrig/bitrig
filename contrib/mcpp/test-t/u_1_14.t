/* u_1_14.t:    Undefined behaviors on undefined #line syntax.  */

/* u.1.14:  #line directive without an argument of line number. */
#line   "filename"

/* u.1.15:  #line directive with the second argument of other than string
        literal.    */
#line   1234    filename

/* u.1.16:  Excessive argument in #line directive.  */
#line   2345    "filename"  Junk

/*  14; "u_1_14.t"; or other undefined results. */
    __LINE__; __FILE__;

