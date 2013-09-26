/* e_ucn.t:     Errors of Universal-character-name sequense.    */

#define macro\U0000001F /* violation of constraint  */
#define macro\uD800     /* violation of constraint (only C, not for C++)    */
#define macro\u123      /* too short sequence (violation of syntax rule)    */
#define macro\U1234567  /* also too short sequence  */

