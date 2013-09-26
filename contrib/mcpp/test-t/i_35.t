/* i_35.t:  Multi-character character constant. */

/* In ASCII character set.  */
/* 35.1:    */
#if     ('ab' != '\x61\x62') || ('\aa' != '\7\x61')
    Bad handling of multi-character character constant.
#endif

