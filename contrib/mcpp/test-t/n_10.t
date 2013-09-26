/* n_10.t:  #if, #elif, #else and #endif pp-directive.  */

#define MACRO_0     0
#define MACRO_1     1

/* 10.1:    */
/* Note: an undefined identifier in #if expression is replaced to 0.    */
/*  1;  */
#if     a
    a;
#elif   MACRO_0
    MACRO_0;
#elif   MACRO_1         /* Valid block  */
    MACRO_1;
#else
    0;
#endif

/* 10.2:    Comments must be processed even if in skipped #if block.    */
/* At least tokenization of string literal and character constant is necessary
        to process comments, e.g. /* is not a comment mark in string literal.
 */
#ifdef  UNDEFINED
    /* Comment  */
    "in literal /* is not a comment"
#endif

