/* n_ppnum.t:   Preprocessing number token *p+* */
/* Undefined on C90, because '+A' is not a valid pp-token.  */

#define A   3
#define glue( a, b) a ## b
/*  12p+A;  */
    glue( 12p+, A);

