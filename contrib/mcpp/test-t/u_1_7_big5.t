/* u_1_7_big5.t:    Invalid multi-byte character sequence (in string literal,
        character constant, header-name or comment).    */

#define str( a)     # a
#pragma setlocale( "chinese-traditional")   /* For Visual C */
#pragma __setlocale( "big5")                /* For MCPP     */

    str( "°Å");   /* 0xa181   */

