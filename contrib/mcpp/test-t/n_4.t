/* n_4.t:   Special tokens. */

/* 4.1: Digraph spellings in directive line.    */
/*  "abc";  */
%: define  stringize( a)    %: a

    stringize( abc);

/* 4.2: Digraph spellings are retained in stringization.    */
/*  "<:";   */
    stringize( <:);

