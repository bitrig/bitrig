/* m_34_gb.t:   Multi-byte character constant encoded in GB-2312.   */

/* 34.1:    */

#pragma __setlocale( "gb2312")              /* For MCPP     */
#pragma setlocale( "chinese-simplified")    /* For Visual C */

#if     '×Ö' == '\xd7\xd6'
    Multi-byte character is encoded in GB-2312.
#else
    I cannot understand GB-2312.
#endif

