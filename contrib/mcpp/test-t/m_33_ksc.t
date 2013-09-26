/* m_33_ksc.t:  Wide character constant encoded in KSC-5601.    */

#include    <limits.h>
#define     BYTES_VAL   (1 << CHAR_BIT)

/* 33.1:    L'ch'.  */

#pragma __setlocale( "ksc5601")             /* For MCPP     */
#pragma setlocale( "korean")                /* For Visual C */

#if     L'í®' == '\xed' * BYTES_VAL + '\xae'
    Wide character is encoded in KSC-5601.
#elif   L'í®' == '\xae' * BYTES_VAL + '\xed'
    Wide character is encoded in KSC-5601.
    Inverted order of evaluation.
#else
    I cannot understand KSC-5601.
#endif
#if     L'í®' < 0
    Evaluated in negative value.
#endif

