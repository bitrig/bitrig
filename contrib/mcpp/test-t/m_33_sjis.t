/* m_33_sjis.t: Wide character constant encoded in shift-JIS.   */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "sjis")                /* For MCPP     */
#pragma setlocale( "japanese")              /* For Visual C */

#if     L'Žš' == '\x8e' * BYTES_VAL + '\x9a'
    Wide character is encoded in shift-JIS.
#elif   L'Žš' == '\x9a' * BYTES_VAL + '\x8e'
    Wide character is encoded in shift-JIS.
    Inverted order of evaluation.
#else
    I cannot understand shift-JIS.
#endif
#if     L'Žš' < 0
    Evaluated in negative value.
#endif

