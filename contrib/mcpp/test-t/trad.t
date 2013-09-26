/* trad.t
 *  Samples for a very old "Reiser" model preprocessor.
 */

#define glue(a, b)  a/**/b
#define xglue(a, b) glue(a,b)
#define ctrl( c)    'c' & 0x1f

#define debug(n1,n2)    printf("n1= %d, n2= %s", x/**/n1, x/**/n2)

/* ISO C preprocessor expands to    :a b c;
 * very old preprocessor does to    :abc;
 */
    glue( glue( a,b),c);

/* ISO C        :a b c;
 * very old     :abc;
 */
    xglue( xglue( a,b),c);

#define abc ABC

/* ISO C preprocessor expands to    :a b c;
 * very old preprocessor does to    :ABC;
 */
    glue( glue( a,b),c);

/* ISO C        :'c' & 0x1f;
 * very old     :'A' & 0x1f;
 */
    ctrl( A);

/* ISO C        :printf("n1= %d, n2= %s", x 1, x 2);
 * very old     :printf("1= %d, 2= %s", x1, x2);
 */
    debug(1,2);

/* ISO C        :text other than comment after #else, #endif line is error
 * very old     :the text is skipped quietly
 */
#define OLD_PREPROCESSOR    1
#if     OLD_PREPROCESSOR
#else   OLD_PREPROCESSOR
#endif  OLD_PREPROCESSOR

/* ISO C     :Token error
 * very old  :Implicit closing quote at end of line
 */
asm("
    .text
_probeintr:
    ss
    incl    _npx_intrs_while_probing
    pushl   %eax
    movb    $0x20,%al
#ifdef PC98
    outb    %al,$0x08
    outb    %al,$0x0
#else
    outb    %al,$0xa0
    outb    %al,$0x20
#endif
");

