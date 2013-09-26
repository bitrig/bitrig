/* u_1_8.t:     Undefined behaviors on unterminated quotations (1). */

/* u.1.8:   Unterminated character constant.    */
/*  The following "comment" may not interpreted as a comment but swallowed by
        the unterminated character constant.    */
#error  I can't understand. /* Token error prior to execution of #error.    */

