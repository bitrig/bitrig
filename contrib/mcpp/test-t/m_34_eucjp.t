/* m_34_eucjp.t:    Multi-byte character constant encoded in EUC-JP.    */

/* 34.1:    */

#pragma __setlocale( "eucjp")               /* For MCPP     */
#pragma setlocale( "eucjp")                 /* For MCPP on VC   */

#if     '»ú' == '\xbb\xfa'
    Multi-byte character is encoded in EUC-JP.
#else
    I cannot understand EUC-JP.
#endif

