/* n_28.t:  __FILE__, __LINE__, __DATE__, __TIME__, __STDC__ and
            __STDC_VERSION__ are predefined.    */

/* 28.1:    */
/*  "n_28.t";   */
    __FILE__;

/* 28.2:    */
/*  10; */
    __LINE__;

/* 28.3:    */
/*  "Aug  1 2001";  */
    __DATE__;

/* 28.4:    */
/*  "21:42:22"; */
    __TIME__;

/* 28.5:    */
/*  1;  */
    __STDC__;

/* 28.6:    */
/*  199409L;    */
/* In C99, the value of this macro is 199901L   */
    __STDC_VERSION__;

/* 28.7:    __LINE__, __FILE__ in an included file. */
/*  3; "line.h";    */
#include    "line.h"

