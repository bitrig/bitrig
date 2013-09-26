/* n_15.t:  #ifdef, #ifndef directives. */

#define MACRO_1     1

/* 15.1:    #ifdef directive.   */
/*  Valid block */
#ifdef  MACRO_1
    Valid block
#else
    Block to be skipped
#endif

/* 15.2:    #ifndef directive.  */
/*  Valid block */
#ifndef MACRO_1
    Block to be skipped
#else
    Valid block
#endif

