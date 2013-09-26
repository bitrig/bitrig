/* m_33_gb.t:   Wide character constant encoded in GB-2312. */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "gb2312")              /* For MCPP     */
#pragma setlocale( "chinese-simplified")    /* For Visual C */

#if     L'×Ö' == '\xd7' * BYTES_VAL + '\xd6'
    Wide character is encoded in GB 2312.
#elif   L'×Ö' == '\xd6' * BYTES_VAL + '\xd7'
    Wide character is encoded in GB 2312.
    Inverted order of evaluation.
#else
    I cannot understand GB-2312.
#endif
#if     L'×Ö' < 0
    Evaluated in negative value.
#endif

