/* m_33_utf8.t: Wide character constant encoded in UTF-8.   */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "utf8")                /* For MCPP     */
#pragma setlocale( "utf8")                  /* For MCPP on VC   */

#if     L'字' == '\xe5' * BYTES_VAL * BYTES_VAL + '\xad' * BYTES_VAL + '\x97'
    Wide character is encoded in UTF-8.
#elif   L'字' == '\x97' * BYTES_VAL * BYTES_VAL + '\xad' * BYTES_VAL + '\xe5'
    Wide character is encoded in UTF-8.
    Inverted order of evaluation.
#else
    I cannot understand UTF-8.
#endif
#if     L'字' < 0
    Evaluated in negative value.
#endif

