/* m_34_sjis.t: Multi-byte character constant encoded in shift-JIS. */

/* 34.1:    */

#pragma __setlocale( "sjis")                /* For MCPP     */
#pragma setlocale( "japanese")              /* For Visual C */

#if     'Žš' == '\x8e\x9a'
    Multi-byte character is encoded in shift-JIS.
#else
    I cannot understand shift-JIS.
#endif

