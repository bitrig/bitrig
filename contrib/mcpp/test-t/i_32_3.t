/* i_32_3.t:    Character constant in #if expression.   */

/* In ASCII character set.  */
/* 32.3:    */
#if     'a' != 0x61
    Not ASCII character set, or bad evaluation of character constant.
#endif

/* 32.4:    '\a' and '\v'   */
#if     '\a' != 7 || '\v' != 11
    Not ASCII character set, or bad evaluation of escape sequences.
#endif

