/* n_13_5.t:    Arithmetic conversion in #if expressions.   */

/* 13.5:    The usual arithmetic conversion is not performed on bit shift.  */
#if     -1 << 3U > 0
    Bad conversion of bit shift operands.
#endif

/* 13.6:    Usual arithmetic conversions.   */
#if     -1 <= 0U        /* -1 is converted to unsigned long.    */
    Bad arithmetic conversion.
#endif

#if     -1 * 1U <= 0
    Bad arithmetic conversion.
#endif

/* Second and third operands of conditional operator are converted to the
        same type, thus -1 is converted to unsigned long.    */
#if     (1 ? -1 : 0U) <= 0
    Bad arithmetic conversion.
#endif

