/* u_1_7_jis.t: Invalid multi-byte character sequence (in string literal,
        character constant, header-name or comment).    */

#define str( a)     # a
#pragma __setlocale( "jis")                 /* For MCPP     */

    str( "$B1 (B");   /* 0x3120   */

