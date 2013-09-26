/* n_nularg.t:  Empty argument of macro call.   */

#define ARG( a)         # a
#define EMPTY
#define SHOWN( n)       printf( "%s : %d\n", # n, n)
#define SHOWS( s)       printf( "%s : %s\n", # s, ARG( s))
#define add( a, b)      (a + b)
#define sub( a, b)      (a - b)
#define math( op, a, b)     op( a, b)
#define APPEND( a, b)       a ## b

/*  printf( "%s : %d\n", "math( sub, , y)", ( - y));    */
    SHOWN( math( sub, , y));

/*  printf( "%s : %s\n", "EMPTY", "");  */
    SHOWS( EMPTY);

/*  printf( "%s : %s\n", "APPEND( CON, 1)", "CON1");    */
    SHOWS( APPEND( CON, 1));

/*  printf( "%s : %s\n", "APPEND( CON, )", "CON");  */
    SHOWS( APPEND( CON, ));

/*  printf( "%s : %s\n", "APPEND( , )", "");  */
    SHOWS( APPEND( , ));

