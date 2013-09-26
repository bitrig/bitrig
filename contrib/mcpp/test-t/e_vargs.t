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

