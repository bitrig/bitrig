/* n_5.t:   Spaces or tabs are allowed at any place in pp-directive line,
        including between the top of a pp-directive line and '#', and between
        the '#' and the directive. */

/* 5.1: */
/*  |**|[TAB]# |**|[TAB]define |**| MACRO_abcde[TAB]|**| abcde |**| */
/**/	# /**/	define /**/ MACRO_abcde	/**/ abcde /**/
/*  abcde   */
    MACRO_abcde

