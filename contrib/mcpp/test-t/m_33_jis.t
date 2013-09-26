/* m_33_jis.t:  Wide character constant encoded in ISO-2022-JP. */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "jis")                 /* For MCPP     */
#pragma setlocale( "jis")                   /* For MCPP on VC   */

#if     L'字' == 0x3b * BYTES_VAL + 0x7a
    /* This line doesn't work unless "shift states" are processed.  */
    Wide character is encoded in ISO-2022-JP.
#elif   L'字' == 0x7a * BYTES_VAL + 0x3b
    /* This line doesn't work unless "shift states" are processed.  */
    Wide character is encoded in ISO-2022-JP.
    Inverted order of evaluation.
#else
    I cannot understand ISO-2022-JP.
#endif
#if     L'字' < 0
    Evaluated in negative value.
#endif

