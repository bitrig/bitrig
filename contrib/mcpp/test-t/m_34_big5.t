/* m_34_big5.t: Multi-byte character constant encoded in Big-Five.  */

/* 34.1:    */

#pragma __setlocale( "big5")                /* For MCPP     */
#pragma setlocale( "chinese-traditional")   /* For Visual C */

#if     '¦r' == '\xa6\x72'
    Multi-byte character is encoded in Big-Five.
#else
    I cannot understand Big-Five.
#endif

