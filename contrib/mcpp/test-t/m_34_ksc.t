/* m_34_ksc.t:  Multi-byte character constant encoded in KSC-5601.  */

/* 34.1:    */

#pragma __setlocale( "ksc5601")             /* For MCPP     */
#pragma setlocale( "korean")                /* For Visual C */

#if     'í®' == '\xed\xae'
    Multi-byte character is encoded in KSC-5601.
#else
    I cannot understand KSC-5601.
#endif

