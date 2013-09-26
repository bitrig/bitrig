/* u_1_7_utf8.t:    Invalid multi-byte character sequence (in string literal,
        character constant, header-name or comment).    */

#define str( a)     # a
#pragma __setlocale( "utf8")                /* For MCPP     */

    str( "å­—");   /* 0xe5ad97 : legal */
    str( "À¯");   /* 0xc0af   : overlong  */
    str( "àŸ¿");   /* 0xe09fbf : overlong  */
    str( "í €");   /* 0xeda080 : UTF-16 surrogate  */

