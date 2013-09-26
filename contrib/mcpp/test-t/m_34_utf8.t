/* m_34_utf8.t: Multi-byte character constant encoded in UTF-8. */

/* 34.1:    */

#pragma __setlocale( "utf8")                /* For MCPP     */
#pragma setlocale( "utf8")                  /* For MCPP on VC   */

#if     '字' == '\xe5\xad\x97'
    Multi-byte character is encoded in UTF-8.
#else
    I cannot understand UTF-8.
#endif

