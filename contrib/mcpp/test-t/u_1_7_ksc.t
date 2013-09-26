/* u_1_7_ksc.t: Invalid multi-byte character sequence (in string literal,
        character constant, header-name or comment).    */

#define str( a)     # a
#pragma setlocale( "korean")                /* For Visual C */
#pragma __setlocale( "ksc5601")             /* For MCPP     */

    str( "± ");   /* 0xb1a0   */

