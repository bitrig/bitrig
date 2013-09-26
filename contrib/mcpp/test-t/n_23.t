/* n_23.t:  ## operator in macro definition.    */

#define glue( a, b)     a ## b
#define xglue( a, b)    glue( a, b)

/* 23.1:    */
/*  xy; */
    glue( x, y);

/* 23.2:    Generate a preprocessing number.    */
/*  .12e+2; */
#define EXP     2
    xglue( .12e+, EXP);

