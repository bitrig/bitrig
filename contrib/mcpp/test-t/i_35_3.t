/* i_35_3.t:    Multi-character wide character constant.    */

/* In ASCII character set.  */
/* 35.3:    */
#if     (L'ab' != L'\x61\x62') || (L'ab' == 'ab')
    Bad handling of multi-character wide character constant.
#endif

