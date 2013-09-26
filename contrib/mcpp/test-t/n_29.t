/* n_29.t:  #undef directive.   */

/* 29.1:    Undefined macro is not a macro. */
/*  DEFINED;    */
#define DEFINED
#undef  DEFINED
    DEFINED;

/* 29.2:    Undefining undefined name is not an error.  */
#undef  UNDEFINED

