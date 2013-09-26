/* n_21.t:  Tokenization (No preprocessing tokens are merged implicitly).   */

/* 21.1:    */
/*  - - -a; */
#define MINUS   -
    -MINUS-a;

/* 21.2:    */
#define sub( a, b)  a-b     /* '(a)-(b)' is better  */
#define Y   -y              /* '(-y)' is better     */
/*  x- -y;  */
    sub( x, Y);

