/* n_vargs.t:   Macro of variable arguments */
/* from C99 Standard 6.10.3 Examples    */
    #define debug( ...) fprintf( stderr, __VA_ARGS__)
    #define showlist( ...)  puts( #__VA_ARGS__)
    #define report( test, ...)  ((test) ? puts( #test)  \
            : printf( __VA_ARGS__))
    {
        /* fprintf( stderr, "Flag");    */
    debug( "Flag");
        /* fprintf( stderr, "X = %d\n", x);     */
    debug( "X = %d\n", x);
        /* puts( "The first, second, and third items.");   */
    showlist( The first, second, and third items.);
        /* ((x>y) ? puts( "x>y") : printf( "x is %d but y is %d", x, y));   */
    report( x>y, "x is %d but y is %d", x, y);
    }

