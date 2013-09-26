/* n_llong.t:   long long in #if expression */

#if 12345678901234567890 < 13345678901234567890
    "long long #if expression is implemented."
#else
    "long long #if expression is not implemented."
#endif

#if 12345678901234567890LL < 13345678901234567890LL
    Valid block
#else
    Block to be skipped
#endif

#if (0x7FFFFFFFFFFFFFFFULL - 0x6FFFFFFFFFFFFFFFULL) >> 60 == 1
    Valid block
#else
    Block to be skipped
#endif

