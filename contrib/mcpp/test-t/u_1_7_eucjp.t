/* u_1_7_eucjp.t:   Invalid multi-byte character sequence (in string literal,
        character constant, header-name or comment).    */

#define str( a)     # a
#pragma __setlocale( "eucjp")               /* For MCPP     */

    str( "± ");   /* 0xb1a0   */

