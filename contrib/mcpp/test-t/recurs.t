/* recurs.t:    recursive macro */
/* The sample posted to comp.std.c by B. Stroustrap.    */

#define NIL(xxx)    xxx
#define G_0(arg)    NIL(G_1)(arg)
#define G_1(arg)    NIL(arg)

G_0(42)

/*
 * Note by kmatsui:
 * There are two interpretations on the Standard's specification.
 * (1)  This macro should be expanded to 'NIL(42)'.
 * (2)  This macro should be expanded to '42'.
 * The Standard's wording seems to justify the (1).
 * GCC, Visual C++ and other major implementations, however, expand
 * this macro as (2).
 * MCPP V.2.4.1 or later of Standard mode expands this as (1) by default,
 * and expands as (2) when invoked with -@compat option.
 */
 
