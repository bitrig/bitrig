/* e_14_10.t:   Overflow of constant expression in #if directive.    */

/* 14.10:   */
/* In C99, #if expression is evaluated in intmax_t  */
#if __STDC_VERSION__ < 199901L
#include    <limits.h>

#if     LONG_MAX - LONG_MIN
#endif
#if     LONG_MAX + 1 > SHRT_MAX
#endif
#if     LONG_MIN - 1
#endif
#if     LONG_MAX * 2
#endif
#endif

