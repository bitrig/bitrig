/* m_36_big5.t: Handling of 0x5c in BigFive multi-byte character.   */

#define     str( a)     # a

/* 36.1:    0x5c in multi-byte character is not an escape character */

#pragma __setlocale( "big5")                /* For MCPP     */
#pragma setlocale( "chinese-traditional")   /* For Visual C */

#if     '¦r' == '\xa6\x72' && '¥\' != '\xa5\x5c'
    Bad handling of '\\' in multi-byte character.
#endif

/* 36.ext:  */
    "¥\ÁZ";   /* \xa5\x5c\xc1\x5a */

/* 36.2:    # operater should not insert '\\' before 0x5c in multi-byte
        character.  */
    str( "¥\ÁZ");

