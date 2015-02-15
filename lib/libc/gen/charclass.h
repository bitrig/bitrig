/*
 * Public domain, 2008, Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * $OpenBSD: charclass.h,v 1.1 2008/10/01 23:04:13 millert Exp $
 */

/*
 * POSIX character class support for fnmatch() and glob().
 */
static struct cclass {
	const char *name;
	int (*isctype)(int);
	int (*iswctype)(wint_t);
} cclasses[] = {
	{ "alnum",	isalnum,	iswalnum },
	{ "alpha",	isalpha,	iswalpha },
	{ "blank",	isblank,	iswblank },
	{ "cntrl",	iscntrl,	iswcntrl },
	{ "digit",	isdigit,	iswdigit },
	{ "graph",	isgraph,	iswgraph },
	{ "lower",	islower,	iswlower },
	{ "print",	isprint,	iswprint },
	{ "punct",	ispunct,	iswpunct },
	{ "space",	isspace,	iswspace },
	{ "upper",	isupper,	iswupper },
	{ "xdigit",	isxdigit,	iswxdigit },
	{ NULL,		NULL,		NULL }
};

#define NCCLASSES	(sizeof(cclasses) / sizeof(cclasses[0]) - 1)
