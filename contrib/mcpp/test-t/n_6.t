/* n_6.t:   #include directive. */

/* 6.1: Header-name quoted by " and " as well as by < and > can include
        standard headers.   */
/* Note: Standard headers can be included any times.    */
#include    "ctype.h"
#include    <ctype.h>

/* 6.2: Macro is allowed in #include line.  */
#define HEADER  "header.h"
/* Before file inclusion:   #include "header.h" */
#include    HEADER
/*  abc */
    MACRO_abc

/* 6.3: With macro nonsence but legal.  */
#undef  MACRO_abc
#define ZERO_TOKEN
#include    ZERO_TOKEN HEADER ZERO_TOKEN
/*  abc */
    MACRO_abc

