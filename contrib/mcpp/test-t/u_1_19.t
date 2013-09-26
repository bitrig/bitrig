/* u_1_19.t:    Undefined behaviors on undefined #define and #undef syntax. */

/* u.1.19:  A macro expanded to "defined" in #if expression.    */
#define DEFINED     defined
#if     DEFINED DEFINED
#endif

#undef  __linux__
#undef  __arm__
#define __linux__   1
#define HAVE_MREMAP defined(__linux__) && !defined(__arm__)
/* Wrong macro definition.
 * This macro should be defined as follows.
 *  #if defined(__linux__) && !defined(__arm__)
 *  #define HAVE_MREMAP 1
 *  #endif
 */
#if HAVE_MREMAP
    mremap();
#endif

/* u.1.20:  Undefining __FILE__, __LINE__, __DATE__, __TIME__, __STDC__ or
        "defined" in #undef directive.  */
#undef  __LINE__
/*  16; or other undefined result.  */
    __LINE__;

/* u.1.21:  Defining __FILE__, __LINE__, __DATE__, __TIME__, __STDC__ or
        "defined" in #define directive. */
#define __LINE__    1234
/*  18; or other undefined result.  */
    __LINE__;
#define defined     defined
#if     defined defined
#   error   I am not a good preprocessor.
#endif

