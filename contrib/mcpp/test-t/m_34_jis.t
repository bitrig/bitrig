/* m_34_jis.t:  Multi-byte character constant encoded in ISO-2022-JP.   */

/* 34.1:    */

#pragma __setlocale( "jis")                 /* For MCPP     */
#pragma setlocale( "jis")                   /* For MCPP on VC   */

#if     '字' == '\x3b\x7a'
    /* This line doesn't work unless "shift states" are processed.  */
    Multi-byte character is encoded in ISO-2022-JP.
#else
    I cannot understand ISO-2022-JP.
#endif

