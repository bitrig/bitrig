/* m_33_big5.t: Wide character constant encoded in Big-Five.    */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "big5")                /* For MCPP     */
#pragma setlocale( "chinese-traditional")   /* For Visual C */

#if     L'¦r' == '\xa6' * BYTES_VAL + '\x72'
    Wide character is encoded in Big-Five.
#elif   L'¦r' == '\x72' * BYTES_VAL + '\xa6'
    Wide character is encoded in Big-Five.
    Inverted order of evaluation.
#else
    I cannot understand Big-Five.
#endif
#if     L'¦r' < 0
    Evaluated in negative value.
#endif

