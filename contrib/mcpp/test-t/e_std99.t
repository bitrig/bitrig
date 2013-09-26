/*
 *      e_std99.t
 *
 * 2002/08      made public                                     kmatsui
 * 2003/11      added a few samples                             kmatsui
 *
 * Samples to test Standard C99 preprocessing.
 * C99 preprocessor must diagnose all of these samples appropriately.
 */


/* e_ucn.t:     Errors of Universal-character-name sequense.    */

#define macro\U0000001F     violation of constraint
#define macro\uD800         violation of constraint (only C, not for C++)
#define macro\u123          too short sequence (violation of syntax rule)
#define macro\U1234567      also too short sequence


/* e_intmax.t:  Overflow of constant expression in #if directive.    */

#include    <stdint.h>

#if     INTMAX_MAX - INTMAX_MIN
#endif
#if     INTMAX_MAX + 1
#endif
#if     INTMAX_MIN - 1
#endif
#if     INTMAX_MAX * 2
#endif

#include	<limits.h>

#if     LLONG_MAX - LLONG_MIN
#endif
#if     LLONG_MAX + 1
#endif
#if     LLONG_MIN - 1
#endif
#if     LLONG_MAX * 2
#endif


/* e_pragma.t:  Erroneous use of _Pragma() operator */
        /* Operand of _Pragma() should be a string literal  */
    _Pragma( This is not a string literal)


/* e_vargs1.t:  Erroneous usage of __VA_ARGS__  */

/* __VA_ARGS__ should not be defined.   */
    #define __VA_ARGS__ (x, y, z)

/*
 * __VA_ARGS__ should be the parameter name in replacement list
 * corresponding to '...'.
 */
    #define wrong_macro( a, b, __VA_ARGS__) (a + b - __VA_ARGS__)


/* e_vargs2.t:  Erroneous macro invocation of variable arguments    */
    #define debug( ...) fprintf( stderr, __VA_ARGS__)
        /* No argument to correspond __VA_ARGS__    */
    debug();

