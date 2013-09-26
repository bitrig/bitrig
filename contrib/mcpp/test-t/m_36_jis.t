/*
 * m_36_jis.t:  Handling of 0x5c ('\\'), 0x22 ('"'), 0x27 ('\'')
 * in ISO-2022-JP multi-byte character.
 */

#define     str( a)     # a

/* 36.1:    0x5c in multi-byte character is not an escape character.    */

#pragma __setlocale( "jis")                 /* For MCPP     */
#pragma setlocale( "jis")                   /* For MCPP on VC   */

#if     '字' == '\x3b\x7a' && '移' != '\x30\x5c'
    Bad handling of '\\' in multi-byte character.
#endif

/* 36.2:    # operater should not insert '\\' before 0x5c in multi-byte
        character.  */
    str( "移動");
   /* \x22\x5c\x22\x30\x5c\x46\x30\x5c\x22\x22 with shift-sequence  */

/* 36.3:    # operater should not insert '\\' before 0x22 in multi-byte
        character.  */
    str( "陰陽");
   /* \x22\x5c\x22\x31\x22\x4d\x5b\x5c\x22\x22 with shift-sequence  */

/* 36.4:    # operater should not insert '\\' before 0x27 in multi-byte
        character.  */
    str( "宇宙");
   /* \x22\x5c\x22\x31\x27\x43\x68\x5c\x22\x22 with shift-sequence  */

