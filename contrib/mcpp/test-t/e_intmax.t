/* e_intmax.t:  Overflow of constant expression in #if directive.    */

#include	<limits.h>
#include    <stdint.h>

#if     INTMAX_MAX - INTMAX_MIN
#endif
#if     INTMAX_MAX + 1 > SHRT_MAX
#endif
#if     INTMAX_MIN - 1
#endif
#if     INTMAX_MAX * 2
#endif

#if     LLONG_MAX - LLONG_MIN
#endif
#if     LLONG_MAX + 1 > SHRT_MAX
#endif
#if     LLONG_MIN - 1
#endif
#if     LLONG_MAX * 2
#endif

