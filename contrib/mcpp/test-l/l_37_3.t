/* l_37_3.t:    Translation limits larger than Standard / 3.    */

/* 37.3L:   Significant initial characters in an internal identifier or a
        macro name. */

/*  Name of 127 bytes long. */
    int
A123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
B123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde = 127;
#ifndef X7F
/*  Name of 255 bytes long. */
    int
A123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
B123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
C123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\
D123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde = 255;
#endif

