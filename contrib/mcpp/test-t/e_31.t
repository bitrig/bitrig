/* e_31.t:  Illegal macro calls.    */

#define sub( a, b)      (a - b)

/* 31.1:    Too many arguments error.   */
    sub( x, y, z);

/* 31.2:    Too few arguments error.    */
    sub( x);

