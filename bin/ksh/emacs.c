/*	$OpenBSD: emacs.c,v 1.49 2015/02/16 01:44:41 tedu Exp $	*/

/*
 *  Emacs-like command line editing and history
 *
 *  created by Ron Natalie at BRL
 *  modified by Doug Kingston, Doug Gwyn, and Lou Salkind
 *  adapted to PD ksh by Eric Gisin
 *
 * partial rewrite by Marco Peereboom <marco@openbsd.org>
 * under the same license
 *
 * multiline editing by Martin Natano <natano@natano.net>
 * under the same license
 */

#include "config.h"
#ifdef EMACS

#include "sh.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>
#include "edit.h"

static	Area	aedit;
#define	AEDIT	&aedit		/* area for kill ring and macro defns */

#define	CTRL(x)		((x) == '?' ? 0x7F : (x) & 0x1F)	/* ASCII */
#define	UNCTRL(x)	((x) == 0x7F ? '?' : (x) | 0x40)	/* ASCII */
#define	META(x)		((x) & 0x7f)
#define	ISMETA(x)	(Flag(FEMACSUSEMETA) && ((x) & 0x80))


/* values returned by keyboard functions */
#define	KSTD	0
#define	KEOL	1		/* ^M, ^J */
#define	KINTR	2		/* ^G, ^C */

struct	x_ftab {
	int		(*xf_func)(int c);
	const char	*xf_name;
	short		xf_flags;
};

#define XF_ARG		1	/* command takes number prefix */
#define	XF_NOBIND	2	/* not allowed to bind to function */
#define	XF_PREFIX	4	/* function sets prefix */

/* Separator for completion */
#define	is_cfs(c)	(c == ' ' || c == '\t' || c == '"' || c == '\'')

/* Separator for motion */
#define	is_mfs(c)	(!(isalnum((unsigned char)c) || c == '_' || c == '$'))

/* Arguments for do_complete()
 * 0 = enumerate  M-= complete as much as possible and then list
 * 1 = complete   M-Esc
 * 2 = list       M-?
 */
typedef enum {
	CT_LIST,	/* list the possible completions */
	CT_COMPLETE,	/* complete to longest prefix */
	CT_COMPLIST	/* complete and then list (if non-exact) */
} Comp_type;

/* keybindings */
struct kb_entry {
	TAILQ_ENTRY(kb_entry)	entry;
	char			*seq;
	int			len;
	struct x_ftab		*ftab;
	void			*args;
};
TAILQ_HEAD(kb_list, kb_entry);
struct kb_list			kblist = TAILQ_HEAD_INITIALIZER(kblist);

/* { from 4.9 edit.h */
/*
 * The following are used for my horizontal scrolling stuff
 */
static char    *xbuf;		/* beg input buffer */
static char    *xend;		/* end input buffer */
static char    *xcp;		/* current position */
static char    *xep;		/* current end */

static int	x_col;
static int	x_arg;		/* general purpose arg */
static int	x_arg_defaulted;/* x_arg not explicitly set; defaulted to 1 */

/* end from 4.9 edit.h } */
static	int	x_tty;		/* are we on a tty? */
static	int	x_bind_quiet;	/* be quiet when binding keys */
static int	(*x_last_command)(int);

static	char   **x_histp;	/* history position */
static	int	x_nextcmd;	/* for newline-and-next */
static	char	*xmp;		/* mark pointer */
#define	KILLSIZE	20
static	char	*killstack[KILLSIZE];
static	int	killsp, killtp;
static	int	x_literal_set;
static	int	x_arg_set;
static	char	*macro_args;
static	int	prompt_skip;

static void	x_cs(const char *, ...);
static int	x_ins(char *);
static void	x_delete(int, int);
static int	x_bword(void);
static int	x_fword(void);
static void	x_goto(char *);
static void	x_bs(int);
static int	x_size(int);
static void	x_zots(char *);
static void	x_zotc(int);
static void	x_load_hist(char **);
static int	x_search(char *, int);
static int	x_match(char *, char *);
static void	x_redraw(int);
static void	x_push(int);
static void	x_e_ungetc(int);
static int	x_e_getc(int);
static void	x_e_putc(int);
static void	x_e_puts(const char *);
static int	x_comment(int);
static int	x_fold_case(int);
static void	do_complete(int, Comp_type);
static int	x_emacs_putbuf(const char *, size_t);

/* proto's for keybindings */
static int	x_abort(int);
static int	x_beg_hist(int);
static int	x_comp_comm(int);
static int	x_comp_file(int);
static int	x_complete(int);
static int	x_del_back(int);
static int	x_del_bword(int);
static int	x_del_char(int);
static int	x_del_fword(int);
static int	x_del_line(int);
static int	x_draw_line(int);
static int	x_end_hist(int);
static int	x_end_of_text(int);
static int	x_enumerate(int);
static int	x_eot_del(int);
static int	x_error(int);
static int	x_goto_hist(int);
static int	x_ins_string(int);
static int	x_insert(int);
static int	x_kill(int);
static int	x_kill_region(int);
static int	x_list_comm(int);
static int	x_list_file(int);
static int	x_literal(int);
static int	x_meta_yank(int);
static int	x_mv_back(int);
static int	x_mv_begin(int);
static int	x_mv_bword(int);
static int	x_mv_end(int);
static int	x_mv_forw(int);
static int	x_mv_fword(int);
static int	x_newline(int);
static int	x_next_com(int);
static int	x_nl_next_com(int);
static int	x_noop(int);
static int	x_prev_com(int);
static int	x_prev_histword(int);
static int	x_search_char_forw(int);
static int	x_search_char_back(int);
static int	x_search_hist(int);
static int	x_set_mark(int);
static int	x_stuff(int);
static int	x_stuffreset(int);
static int	x_transpose(int);
static int	x_version(int);
static int	x_xchg_point_mark(int);
static int	x_yank(int);
static int	x_comp_list(int);
static int	x_expand(int);
static int	x_fold_capitalize(int);
static int	x_fold_lower(int);
static int	x_fold_upper(int);
static int	x_set_arg(int);
static int	x_comment(int);
#ifdef DEBUG
static int	x_debug_info(int);
#endif

static const struct x_ftab x_ftab[] = {
	{ x_abort,		"abort",			0 },
	{ x_beg_hist,		"beginning-of-history",		0 },
	{ x_comp_comm,		"complete-command",		0 },
	{ x_comp_file,		"complete-file",		0 },
	{ x_complete,		"complete",			0 },
	{ x_del_back,		"delete-char-backward",		XF_ARG },
	{ x_del_bword,		"delete-word-backward",		XF_ARG },
	{ x_del_char,		"delete-char-forward",		XF_ARG },
	{ x_del_fword,		"delete-word-forward",		XF_ARG },
	{ x_del_line,		"kill-line",			0 },
	{ x_draw_line,		"redraw",			0 },
	{ x_end_hist,		"end-of-history",		0 },
	{ x_end_of_text,	"eot",				0 },
	{ x_enumerate,		"list",				0 },
	{ x_eot_del,		"eot-or-delete",		XF_ARG },
	{ x_error,		"error",			0 },
	{ x_goto_hist,		"goto-history",			XF_ARG },
	{ x_ins_string,		"macro-string",			XF_NOBIND },
	{ x_insert,		"auto-insert",			XF_ARG },
	{ x_kill,		"kill-to-eol",			XF_ARG },
	{ x_kill_region,	"kill-region",			0 },
	{ x_list_comm,		"list-command",			0 },
	{ x_list_file,		"list-file",			0 },
	{ x_literal,		"quote",			0 },
	{ x_meta_yank,		"yank-pop",			0 },
	{ x_mv_back,		"backward-char",		XF_ARG },
	{ x_mv_begin,		"beginning-of-line",		0 },
	{ x_mv_bword,		"backward-word",		XF_ARG },
	{ x_mv_end,		"end-of-line",			0 },
	{ x_mv_forw,		"forward-char",			XF_ARG },
	{ x_mv_fword,		"forward-word",			XF_ARG },
	{ x_newline,		"newline",			0 },
	{ x_next_com,		"down-history",			XF_ARG },
	{ x_nl_next_com,	"newline-and-next",		0 },
	{ x_noop,		"no-op",			0 },
	{ x_prev_com,		"up-history",			XF_ARG },
	{ x_prev_histword,	"prev-hist-word",		XF_ARG },
	{ x_search_char_forw,	"search-character-forward",	XF_ARG },
	{ x_search_char_back,	"search-character-backward",	XF_ARG },
	{ x_search_hist,	"search-history",		0 },
	{ x_set_mark,		"set-mark-command",		0 },
	{ x_stuff,		"stuff",			0 },
	{ x_stuffreset,		"stuff-reset",			0 },
	{ x_transpose,		"transpose-chars",		0 },
	{ x_version,		"version",			0 },
	{ x_xchg_point_mark,	"exchange-point-and-mark",	0 },
	{ x_yank,		"yank",				0 },
	{ x_comp_list,		"complete-list",		0 },
	{ x_expand,		"expand-file",			0 },
	{ x_fold_capitalize,	"capitalize-word",		XF_ARG },
	{ x_fold_lower,		"downcase-word",		XF_ARG },
	{ x_fold_upper,		"upcase-word",			XF_ARG },
	{ x_set_arg,		"set-arg",			XF_NOBIND },
	{ x_comment,		"comment",			0 },
#ifdef DEBUG
	{ x_debug_info,		"debug-info",			0 },
#endif
};

int
x_emacs(char *buf, size_t len)
{
	struct kb_entry		*k, *kmatch = NULL;
	char			line[LINE + 1];
	int			at = 0, submatch, ret, c;
	const char		*p;

	xbuf = buf; xend = buf + len;
	xcp = xep = buf;
	*xcp = 0;
	xmp = NULL;
	x_histp = histptr + 1;

	x_col = promptlen(prompt, &p);
	prompt_skip = p - prompt;

	pprompt(prompt, 0);

	if (x_nextcmd >= 0) {
		int off = source->line - x_nextcmd;
		if (histptr - history >= off)
			x_load_hist(histptr - off);
		x_nextcmd = -1;
	}

	line[0] = '\0';
	x_literal_set = 0;
	x_arg = -1;
	x_last_command = NULL;
	while (1) {
		x_flush();
		if ((c = x_e_getc(1)) < 0)
			return 0;

		line[at++] = c;
		line[at] = '\0';

		if (x_arg == -1) {
			x_arg = 1;
			x_arg_defaulted = 1;
		}

		if (x_literal_set) {
			/* literal, so insert it */
			x_literal_set = 0;
			submatch = 0;
		} else {
			submatch = 0;
			kmatch = NULL;
			TAILQ_FOREACH(k, &kblist, entry) {
				if (at > k->len)
					continue;

				if (memcmp(k->seq, line, at) == 0) {
					/* sub match */
					submatch++;
					if (k->len == at)
						kmatch = k;
				}

				/* see if we can abort search early */
				if (submatch > 1)
					break;
			}
		}

		if (submatch == 1 && kmatch) {
			if (kmatch->ftab->xf_func == x_ins_string &&
			    kmatch->args && !macro_args) {
				/* treat macro string as input */
				macro_args = kmatch->args;
				ret = KSTD;
			} else
				ret = kmatch->ftab->xf_func(c);
		} else {
			if (submatch)
				continue;
			if (at == 1)
				ret = x_insert(c);
			else
				ret = x_error(c); /* not matched meta sequence */
		}

		switch (ret) {
		case KSTD:
			if (kmatch)
				x_last_command = kmatch->ftab->xf_func;
			else
				x_last_command = NULL;
			break;
		case KEOL:
			ret = xep - xbuf;
			return (ret);
			break;
		case KINTR:
			trapsig(SIGINT);
			x_mode(false);
			unwind(LSHELL);
			x_arg = -1;
			break;
		default:
			bi_errorf("invalid return code"); /* can't happen */
		}

		/* reset meta sequence */
		at = 0;
		line[0] = '\0';
		if (x_arg_set)
			x_arg_set = 0; /* reset args next time around */
		else
			x_arg = -1;
	}
}

static void
x_cs(const char *fmt, ...)
{
	char buf[10];
	va_list ap;

	va_start(ap, fmt);
	(void)vsnprintf(buf, 10, fmt, ap);
	va_end(ap);

	x_putc(CTRL('['));
	x_putc('[');
	x_puts(buf);
}

static int
x_insert(int c)
{
	char	str[2];

	/*
	 *  Should allow tab and control chars.
	 */
	if (c == 0) {
		x_e_putc('\a');
		return KSTD;
	}
	str[0] = c;
	str[1] = '\0';
	while (x_arg--)
		x_ins(str);
	return KSTD;
}

static int
x_ins_string(int c)
{
	return x_insert(c);
}

static int x_do_ins(const char *cp, int len);

static int
x_do_ins(const char *cp, int len)
{
	if (xep+len >= xend) {
		x_e_putc('\a');
		return -1;
	}

	memmove(xcp+len, xcp, xep - xcp + 1);
	memmove(xcp, cp, len);
	xcp += len;
	xep += len;
	return 0;
}

static int
x_ins(char *s)
{
	char	*cp = xcp;

	if (x_do_ins(s, strlen(s)) < 0)
		return -1;
	x_zots(cp);
	for (cp = xep; cp > xcp; )
		x_bs(*--cp);

	return 0;
}

/*
 * this is used for x_escape() in do_complete()
 */
static int
x_emacs_putbuf(const char *s, size_t len)
{
	return x_do_ins(s, len);
}

static int
x_del_back(int c)
{
	int col = xcp - xbuf;

	if (col == 0) {
		x_e_putc('\a');
		return KSTD;
	}
	if (x_arg > col)
		x_arg = col;
	x_goto(xcp - x_arg);
	x_delete(x_arg, false);
	return KSTD;
}

static int
x_del_char(int c)
{
	int nleft = xep - xcp;

	if (!nleft) {
		x_e_putc('\a');
		return KSTD;
	}
	if (x_arg > nleft)
		x_arg = nleft;
	x_delete(x_arg, false);
	return KSTD;
}

/* Delete nc chars to the right of the cursor (including cursor position) */
static void
x_delete(int nc, int push)
{
	int	i,j;
	char	*cp;

	if (nc == 0)
		return;
	if (xmp != NULL && xmp > xcp) {
		if (xcp + nc > xmp)
			xmp = xcp;
		else
			xmp -= nc;
	}

	/*
	 * This lets us yank a word we have deleted.
	 */
	if (push)
		x_push(nc);

	xep -= nc;
	cp = xcp;
	j = 0;
	i = nc;
	while (i--) {
		j += x_size((unsigned char)*cp++);
	}
	memmove(xcp, xcp+nc, xep - xcp + 1);	/* Copies the null */
	x_zots(xcp);
	if ((i = x_cols) > 0) {
		j = (j < i) ? j : i;
		i = j;
		while (i--)
			x_e_putc(' ');
		i = j;
		while (i--)
			x_bs(' ');
	}
	/*x_goto(xcp);*/
	for (cp = xep; cp > xcp; )
		x_bs(*--cp);
}

static int
x_del_bword(int c)
{
	x_delete(x_bword(), true);
	return KSTD;
}

static int
x_mv_bword(int c)
{
	(void)x_bword();
	return KSTD;
}

static int
x_mv_fword(int c)
{
	x_goto(xcp + x_fword());
	return KSTD;
}

static int
x_del_fword(int c)
{
	x_delete(x_fword(), true);
	return KSTD;
}

static int
x_bword(void)
{
	int	nc = 0;
	char	*cp = xcp;

	if (cp == xbuf) {
		x_e_putc('\a');
		return 0;
	}
	while (x_arg--) {
		while (cp != xbuf && is_mfs(cp[-1])) {
			cp--;
			nc++;
		}
		while (cp != xbuf && !is_mfs(cp[-1])) {
			cp--;
			nc++;
		}
	}
	x_goto(cp);
	return nc;
}

static int
x_fword(void)
{
	int	nc = 0;
	char	*cp = xcp;

	if (cp == xep) {
		x_e_putc('\a');
		return 0;
	}
	while (x_arg--) {
		while (cp != xep && is_mfs(*cp)) {
			cp++;
			nc++;
		}
		while (cp != xep && !is_mfs(*cp)) {
			cp++;
			nc++;
		}
	}
	return nc;
}

static void
x_goto(char *cp)
{
	if (cp < xcp) {			/* move back */
		while (cp < xcp)
			x_bs((unsigned char)*--xcp);
	} else if (cp > xcp) {		/* move forward */
		while (cp > xcp)
			x_zotc((unsigned char)*xcp++);
	}
}

static void
x_bs(int c)
{
	int i;

	i = x_size(c);
	while (i--) {
		if (x_col % x_cols == 0) {
			x_cs("A");
			x_cs("%dC", x_cols - 1);
		} else
			x_putc('\b');
		x_col--;
	}
}

static int
x_size(int c)
{
	if (c=='\t')
		return 4;	/* Kludge, tabs are always four spaces. */
	if (iscntrl(c))		/* control char */
		return 2;
	return 1;
}

static void
x_zots(char *str)
{
	while (*str && str < xep)
		x_zotc(*str++);
}

static void
x_zotc(int c)
{
	if (c == '\t') {
		/*  Kludge, tabs are always four spaces.  */
		x_e_puts("    ");
	} else if (iscntrl(c)) {
		x_e_putc('^');
		x_e_putc(UNCTRL(c));
	} else
		x_e_putc(c);
}

static int
x_mv_back(int c)
{
	int col = xcp - xbuf;

	if (col == 0) {
		x_e_putc('\a');
		return KSTD;
	}
	if (x_arg > col)
		x_arg = col;
	x_goto(xcp - x_arg);
	return KSTD;
}

static int
x_mv_forw(int c)
{
	int nleft = xep - xcp;

	if (!nleft) {
		x_e_putc('\a');
		return KSTD;
	}
	if (x_arg > nleft)
		x_arg = nleft;
	x_goto(xcp + x_arg);
	return KSTD;
}

static int
x_search_char_forw(int c)
{
	char *cp = xcp;

	*xep = '\0';
	c = x_e_getc(1);
	while (x_arg--) {
		if (c < 0 ||
		    ((cp = (cp == xep) ? NULL : strchr(cp + 1, c)) == NULL &&
		    (cp = strchr(xbuf, c)) == NULL)) {
			x_e_putc('\a');
			return KSTD;
		}
	}
	x_goto(cp);
	return KSTD;
}

static int
x_search_char_back(int c)
{
	char *cp = xcp, *p;

	c = x_e_getc(1);
	for (; x_arg--; cp = p)
		for (p = cp; ; ) {
			if (p-- == xbuf)
				p = xep;
			if (c < 0 || p == cp) {
				x_e_putc('\a');
				return KSTD;
			}
			if (*p == c)
				break;
		}
	x_goto(cp);
	return KSTD;
}

static int
x_newline(int c)
{
	x_goto(xep);
	x_e_puts("\r\n");
	x_flush();
	*xep++ = '\n';
	return KEOL;
}

static int
x_end_of_text(int c)
{
	x_zotc(edchars.eof);
	x_e_puts("\r\n");
	x_flush();
	return KEOL;
}

static int x_beg_hist(int c) { x_load_hist(history); return KSTD;}

static int x_end_hist(int c) { x_load_hist(histptr); return KSTD;}

static int x_prev_com(int c) { x_load_hist(x_histp - x_arg); return KSTD;}

static int x_next_com(int c) { x_load_hist(x_histp + x_arg); return KSTD;}

/* Goto a particular history number obtained from argument.
 * If no argument is given history 1 is probably not what you
 * want so we'll simply go to the oldest one.
 */
static int
x_goto_hist(int c)
{
	if (x_arg_defaulted)
		x_load_hist(history);
	else
		x_load_hist(histptr + x_arg - source->line);
	return KSTD;
}

static void
x_load_hist(char **hp)
{
	if (hp < history || hp > histptr) {
		x_e_putc('\a');
		return;
	}
	x_histp = hp;
	strlcpy(xbuf, *hp, xend - xbuf);
	xep = xcp = xbuf + strlen(xbuf);
	x_redraw(1);
}

static int
x_nl_next_com(int c)
{
	x_nextcmd = source->line - (histptr - x_histp) + 1;
	return (x_newline(c));
}

static int
x_eot_del(int c)
{
	if (xep == xbuf && x_arg_defaulted)
		return (x_end_of_text(c));
	else
		return (x_del_char(c));
}

static void *
kb_find_hist_func(char c)
{
	struct kb_entry		*k;
	char			line[LINE + 1];

	line[0] = c;
	line[1] = '\0';
	TAILQ_FOREACH(k, &kblist, entry)
		if (!strcmp(k->seq, line))
			return (k->ftab->xf_func);

	return (x_insert);
}

/* reverse incremental history search */
static int
x_search_hist(int c)
{
	int offset = -1;	/* offset of match in xbuf, else -1 */
	char pat [256+1];	/* pattern buffer */
	char *p = pat;
	int (*f)(int);

	*p = '\0';
	set_prompt(ISEARCH, NULL);
	x_redraw(1);
	while (1) {
		x_flush();
		if ((c = x_e_getc(offset >= 0)) < 0)
			return (KSTD);
		f = kb_find_hist_func(c);
		if (c == CTRL('[')) {
			/* might be part of an escape sequence */
			if (x_avail())
				x_e_ungetc(c);
			break;
		} else if (f == x_search_hist)
			offset = x_search(pat, 0);
		else if (f == x_del_back) {
			if (p == pat) {
				offset = -1;
				break;
			}
			if (p > pat)
				*--p = '\0';
			if (p == pat)
				offset = -1;
			else
				offset = x_search(pat, 1);
			continue;
		} else if (f == x_insert) {
			/* add char to pattern */
			/* overflow check... */
			if (p >= &pat[sizeof(pat) - 1]) {
				x_e_putc('\a');
				continue;
			}
			*p++ = c, *p = '\0';
			if (offset >= 0) {
				/* already have partial match */
				offset = x_match(xbuf, pat);
				if (offset >= 0) {
					x_goto(xbuf + offset + (p - pat) -
					    (*pat == '^'));
					continue;
				}
			}
			offset = x_search(pat, 0);
		} else { /* other command */
			x_e_ungetc(c);
			break;
		}
	}
	set_prompt(PS1, NULL);
	x_redraw(1);
	return KSTD;
}

/* search backward from current line */
static int
x_search(char *pat, int sameline)
{
	char **hp;
	int i;

	for (hp = x_histp - (sameline ? 0 : 1) ; hp >= history; --hp) {
		i = x_match(*hp, pat);
		if (i >= 0 && (*x_histp == NULL || strcmp(*x_histp, *hp) != 0)) {
			x_load_hist(hp);
			x_goto(xbuf + i + strlen(pat) - (*pat == '^'));
			return i;
		}
	}
	strlcpy(xbuf, pat, xend - xbuf);
	xep = xcp = xbuf + strlen(xbuf);
	x_redraw(1);
	x_e_putc('\a');
	x_histp = histptr;
	return -1;
}

/* return position of first match of pattern in string, else -1 */
static int
x_match(char *str, char *pat)
{
	if (*pat == '^') {
		return (strncmp(str, pat+1, strlen(pat+1)) == 0) ? 0 : -1;
	} else {
		char *q = strstr(str, pat);
		return (q == NULL) ? -1 : q - str;
	}
}

static int
x_del_line(int c)
{
	int	i;

	*xep = 0;
	i = xep - xbuf;
	xcp = xbuf;
	x_push(i);
	xep = xbuf;
	*xcp = 0;
	xmp = NULL;
	x_redraw(1);
	return KSTD;
}

static int
x_mv_end(int c)
{
	x_goto(xep);
	return KSTD;
}

static int
x_mv_begin(int c)
{
	x_goto(xbuf);
	return KSTD;
}

static int
x_draw_line(int c)
{
	char *cp;

	cp = xcp;
	x_goto(xep);
	xcp = cp;
	x_e_putc('\n');
	x_redraw(0);
	return KSTD;

}

static void
x_redraw(int clr)
{
	int current_row;
	char *cp;

	if (clr) {
		current_row = (x_col + x_cols - 1) / x_cols;
		if (current_row > 1)
			x_cs("%dA", current_row - 1);
		x_e_putc('\r');
		x_cs("0J");
		x_check_sigwinch();
	} else
		x_e_putc('\r');
	x_flush();

	x_col = promptlen(prompt, NULL);
	pprompt(prompt + prompt_skip, 0);
	x_zots(xbuf);
	for (cp = xep; cp > xcp; )
		x_bs(*--cp);
	D__(x_flush();)
}

static int
x_transpose(int c)
{
	char	tmp;

	/* What transpose is meant to do seems to be up for debate. This
	 * is a general summary of the options; the text is abcd with the
	 * upper case character or underscore indicating the cursor position:
	 *     Who			Before	After  Before	After
	 *     at&t ksh in emacs mode:	abCd	abdC   abcd_	(bell)
	 *     at&t ksh in gmacs mode:	abCd	baCd   abcd_	abdc_
	 *     gnu emacs:		abCd	acbD   abcd_	abdc_
	 * Pdksh currently goes with GNU behavior since I believe this is the
	 * most common version of emacs, unless in gmacs mode, in which case
	 * it does the at&t ksh gmacs mode.
	 * This should really be broken up into 3 functions so users can bind
	 * to the one they want.
	 */
	if (xcp == xbuf) {
		x_e_putc('\a');
		return KSTD;
	} else if (xcp == xep || Flag(FGMACS)) {
		if (xcp - xbuf == 1) {
			x_e_putc('\a');
			return KSTD;
		}
		/* Gosling/Unipress emacs style: Swap two characters before the
		 * cursor, do not change cursor position
		 */
		x_bs(xcp[-1]);
		x_bs(xcp[-2]);
		x_zotc(xcp[-1]);
		x_zotc(xcp[-2]);
		tmp = xcp[-1];
		xcp[-1] = xcp[-2];
		xcp[-2] = tmp;
	} else {
		/* GNU emacs style: Swap the characters before and under the
		 * cursor, move cursor position along one.
		 */
		x_bs(xcp[-1]);
		x_zotc(xcp[0]);
		x_zotc(xcp[-1]);
		tmp = xcp[-1];
		xcp[-1] = xcp[0];
		xcp[0] = tmp;
		x_bs(xcp[0]);
		x_goto(xcp + 1);
	}
	return KSTD;
}

static int
x_literal(int c)
{
	x_literal_set = 1;
	return KSTD;
}

static int
x_kill(int c)
{
	int col = xcp - xbuf;
	int lastcol = xep - xbuf;
	int ndel;

	if (x_arg_defaulted)
		x_arg = lastcol;
	else if (x_arg > lastcol)
		x_arg = lastcol;
	ndel = x_arg - col;
	if (ndel < 0) {
		x_goto(xbuf + x_arg);
		ndel = -ndel;
	}
	x_delete(ndel, true);
	return KSTD;
}

static void
x_push(int nchars)
{
	char	*cp = str_nsave(xcp, nchars, AEDIT);
	if (killstack[killsp])
		afree((void *)killstack[killsp], AEDIT);
	killstack[killsp] = cp;
	killsp = (killsp + 1) % KILLSIZE;
}

static int
x_yank(int c)
{
	char *cp;

	if (killsp == 0)
		killtp = KILLSIZE;
	else
		killtp = killsp;
	killtp --;
	if (killstack[killtp] == 0) {
		cp = xcp;
		x_goto(xep);
		x_e_puts("\nnothing to yank\n");
		xcp = cp;
		x_redraw(0);
		return KSTD;
	}
	xmp = xcp;
	x_ins(killstack[killtp]);
	return KSTD;
}

static int
x_meta_yank(int c)
{
	char *cp;
	int len;

	if ((x_last_command != x_yank && x_last_command != x_meta_yank) ||
	    killstack[killtp] == 0) {
		killtp = killsp;
		cp = xcp;
		x_goto(xep);
		x_e_puts("\nyank something first\n");
		xcp = cp;
		x_redraw(0);
		return KSTD;
	}
	len = strlen(killstack[killtp]);
	x_goto(xcp - len);
	x_delete(len, false);
	do {
		if (killtp == 0)
			killtp = KILLSIZE - 1;
		else
			killtp--;
	} while (killstack[killtp] == 0);
	x_ins(killstack[killtp]);
	return KSTD;
}

static int
x_abort(int c)
{
	x_goto(xep);
	/* x_zotc(c); */
	xep = xcp = xbuf;
	*xcp = 0;
	return KINTR;
}

static int
x_error(int c)
{
	x_e_putc('\a');
	return KSTD;
}

static int
x_stuffreset(int c)
{
#ifdef TIOCSTI
	(void)x_stuff(c);
	return KINTR;
#else
	x_zotc(c);
	xcp = xep = xbuf;
	*xcp = 0;
	x_e_putc('\n');
	x_redraw(0);
	return KSTD;
#endif
}

static int
x_stuff(int c)
{
#ifdef TIOCSTI
	char	ch = c;
	bool	savmode = x_mode(false);

	(void)ioctl(TTY, TIOCSTI, &ch);
	(void)x_mode(savmode);
	x_e_putc('\n');
	x_redraw(0);
#endif
	return KSTD;
}

static char *
kb_encode(const char *s)
{
	static char		l[LINE + 1];
	int			at = 0;

	l[at] = '\0';
	while (*s) {
		if (*s == '^') {
			s++;
			if (*s >= '?')
				l[at++] = CTRL(*s);
			else {
				l[at++] = '^';
				s--;
			}
		} else
			l[at++] = *s;
		l[at] = '\0';
		s++;
	}
	return (l);
}

static char *
kb_decode(const char *s)
{
	static char		l[LINE + 1];
	int			i, at = 0;

	l[0] = '\0';
	for (i = 0; i < strlen(s); i++) {
		if (iscntrl(s[i])) {
			l[at++] = '^';
			l[at++] = UNCTRL(s[i]);
		} else
			l[at++] = s[i];
		l[at] = '\0';
	}

	return (l);
}

static int
kb_match(char *s)
{
	int			len = strlen(s);
	struct kb_entry		*k;

	TAILQ_FOREACH(k, &kblist, entry) {
		if (len > k->len)
			continue;

		if (memcmp(k->seq, s, len) == 0)
			return (1);
	}

	return (0);
}

static void
kb_del(struct kb_entry *k)
{
	TAILQ_REMOVE(&kblist, k, entry);
	if (k->args)
		free(k->args);
	afree(k, AEDIT);
}

static struct kb_entry *
kb_add_string(void *func, void *args, char *str)
{
	int			i, count;
	struct kb_entry		*k;
	struct x_ftab		*xf = NULL;

	for (i = 0; i < nitems(x_ftab); i++)
		if (x_ftab[i].xf_func == func) {
			xf = (struct x_ftab *)&x_ftab[i];
			break;
		}
	if (xf == NULL)
		return (NULL);

	if (kb_match(str)) {
		if (x_bind_quiet == 0)
			bi_errorf("duplicate binding for %s", kb_decode(str));
		return (NULL);
	}
	count = strlen(str);

	k = alloc(sizeof *k + count + 1, AEDIT);
	k->seq = (char *)(k + 1);
	k->len = count;
	k->ftab = xf;
	k->args = args ? strdup(args) : NULL;

	strlcpy(k->seq, str, count + 1);

	TAILQ_INSERT_TAIL(&kblist, k, entry);

	return (k);
}

static struct kb_entry *
kb_add(void *func, void *args, ...)
{
	va_list			ap;
	char			l[LINE + 1];
	int			i;

	va_start(ap, args);
	for (i = 0; i < sizeof(l) - 1; i++) {
		l[i] = (char)va_arg(ap, unsigned int);
		if (l[i] == '\0')
			break;
	}
	va_end(ap);

	l[i] = '\0';

	return (kb_add_string(func, args, l));
}

static void
kb_print(struct kb_entry *k)
{
	if (!(k->ftab->xf_flags & XF_NOBIND))
		shprintf("%s = %s\n",
		    kb_decode(k->seq), k->ftab->xf_name);
	else if (k->args) {
		shprintf("%s = ", kb_decode(k->seq));
		shprintf("'%s'\n", kb_decode(k->args));
	}
}

int
x_bind(const char *a1, const char *a2,
	int macro,		/* bind -m */
	int list)		/* bind -l */
{
	int			i;
	struct kb_entry		*k, *kb;
	char			in[LINE + 1];

	if (x_tty == 0) {
		bi_errorf("cannot bind, not a tty");
		return (1);
	}

	if (list) {
		/* show all function names */
		for (i = 0; i < nitems(x_ftab); i++) {
			if (x_ftab[i].xf_name &&
			    !(x_ftab[i].xf_flags & XF_NOBIND))
				shprintf("%s\n", x_ftab[i].xf_name);
		}
		return (0);
	}

	if (a1 == NULL) {
		/* show all bindings */
		TAILQ_FOREACH(k, &kblist, entry)
			kb_print(k);
		return (0);
	}

	snprintf(in, sizeof in, "%s", kb_encode(a1));
	if (a2 == NULL) {
		/* print binding */
		TAILQ_FOREACH(k, &kblist, entry)
			if (!strcmp(k->seq, in)) {
				kb_print(k);
				return (0);
			}
		shprintf("%s = %s\n", kb_decode(a1), "auto-insert");
		return (0);
	}

	if (strlen(a2) == 0) {
		/* clear binding */
		TAILQ_FOREACH_SAFE(k, &kblist, entry, kb)
			if (!strcmp(k->seq, in)) {
				kb_del(k);
				break;
			}
		return (0);
	}

	/* set binding */
	if (macro) {
		/* delete old mapping */
		TAILQ_FOREACH_SAFE(k, &kblist, entry, kb)
			if (!strcmp(k->seq, in)) {
				kb_del(k);
				break;
			}
		kb_add_string(x_ins_string, kb_encode(a2), in);
		return (0);
	}

	/* set non macro binding */
	for (i = 0; i < nitems(x_ftab); i++) {
		if (!strcmp(x_ftab[i].xf_name, a2)) {
			/* delete old mapping */
			TAILQ_FOREACH_SAFE(k, &kblist, entry, kb)
				if (!strcmp(k->seq, in)) {
					kb_del(k);
					break;
				}
			kb_add_string(x_ftab[i].xf_func, NULL, in);
			return (0);
		}
	}
	bi_errorf("%s: no such function", a2);
	return (1);
}

void
x_init_emacs(void)
{
	char *locale;

	x_tty = 1;
	ainit(AEDIT);
	x_nextcmd = -1;

	/* Determine if we can translate meta key or use 8-bit AscII
	 * XXX - It would be nice if there was a locale attribute to
	 * determine if the locale is 7-bit or not.
	 */
	locale = setlocale(LC_CTYPE, NULL);
	if (locale == NULL || !strcmp(locale, "C") || !strcmp(locale, "POSIX"))
		Flag(FEMACSUSEMETA) = 1;

	/* new keybinding stuff */
	TAILQ_INIT(&kblist);

	/* man page order */
	kb_add(x_abort,			NULL, CTRL('G'), 0);
	kb_add(x_mv_back,		NULL, CTRL('B'), 0);
	kb_add(x_mv_back,		NULL, CTRL('X'), CTRL('D'), 0);
	kb_add(x_mv_bword,		NULL, CTRL('['), 'b', 0);
	kb_add(x_beg_hist,		NULL, CTRL('['), '<', 0);
	kb_add(x_mv_begin,		NULL, CTRL('A'), 0);
	kb_add(x_fold_capitalize,	NULL, CTRL('['), 'C', 0);
	kb_add(x_fold_capitalize,	NULL, CTRL('['), 'c', 0);
	kb_add(x_comment,		NULL, CTRL('['), '#', 0);
	kb_add(x_complete,		NULL, CTRL('['), CTRL('['), 0);
	kb_add(x_comp_comm,		NULL, CTRL('X'), CTRL('['), 0);
	kb_add(x_comp_file,		NULL, CTRL('['), CTRL('X'), 0);
	kb_add(x_comp_list,		NULL, CTRL('I'), 0);
	kb_add(x_comp_list,		NULL, CTRL('['), '=', 0);
	kb_add(x_del_back,		NULL, CTRL('?'), 0);
	kb_add(x_del_back,		NULL, CTRL('H'), 0);
	/* x_del_char not assigned by default */
	kb_add(x_del_bword,		NULL, CTRL('['), CTRL('?'), 0);
	kb_add(x_del_bword,		NULL, CTRL('['), CTRL('H'), 0);
	kb_add(x_del_bword,		NULL, CTRL('['), 'h', 0);
	kb_add(x_del_fword,		NULL, CTRL('['), 'd', 0);
	kb_add(x_next_com,		NULL, CTRL('N'), 0);
	kb_add(x_next_com,		NULL, CTRL('X'), 'B', 0);
	kb_add(x_fold_lower,		NULL, CTRL('['), 'L', 0);
	kb_add(x_fold_lower,		NULL, CTRL('['), 'l', 0);
	kb_add(x_end_hist,		NULL, CTRL('['), '>', 0);
	kb_add(x_mv_end,		NULL, CTRL('E'), 0);
	/* how to handle: eot: ^_, underneath copied from original keybindings */
	kb_add(x_end_of_text,		NULL, CTRL('_'), 0);
	kb_add(x_eot_del,		NULL, CTRL('D'), 0);
	/* error */
	kb_add(x_xchg_point_mark,	NULL, CTRL('X'), CTRL('X'), 0);
	kb_add(x_expand,		NULL, CTRL('['), '*', 0);
	kb_add(x_mv_forw,		NULL, CTRL('F'), 0);
	kb_add(x_mv_forw,		NULL, CTRL('X'), 'C', 0);
	kb_add(x_mv_fword,		NULL, CTRL('['), 'f', 0);
	kb_add(x_goto_hist,		NULL, CTRL('['), 'g', 0);
	/* kill-line */
	kb_add(x_del_bword,		NULL, CTRL('W'), 0); /* not what man says */
	kb_add(x_kill,			NULL, CTRL('K'), 0);
	kb_add(x_enumerate,		NULL, CTRL('['), '?', 0);
	kb_add(x_list_comm,		NULL, CTRL('X'), '?', 0);
	kb_add(x_list_file,		NULL, CTRL('X'), CTRL('Y'), 0);
	kb_add(x_newline,		NULL, CTRL('J'), 0);
	kb_add(x_newline,		NULL, CTRL('M'), 0);
	kb_add(x_nl_next_com,		NULL, CTRL('O'), 0);
	/* no-op */
	kb_add(x_prev_histword,		NULL, CTRL('['), '.', 0);
	kb_add(x_prev_histword,		NULL, CTRL('['), '_', 0);
	/* how to handle: quote: ^^ */
	kb_add(x_draw_line,		NULL, CTRL('L'), 0);
	kb_add(x_search_char_back,	NULL, CTRL('['), CTRL(']'), 0);
	kb_add(x_search_char_forw,	NULL, CTRL(']'), 0);
	kb_add(x_search_hist,		NULL, CTRL('R'), 0);
	kb_add(x_set_mark,		NULL, CTRL('['), ' ', 0);
#if defined(TIOCSTI)
	kb_add(x_stuff,			NULL, CTRL('T'), 0);
	/* stuff-reset */
#else
	kb_add(x_transpose,		NULL, CTRL('T'), 0);
#endif
	kb_add(x_prev_com,		NULL, CTRL('P'), 0);
	kb_add(x_prev_com,		NULL, CTRL('X'), 'A', 0);
	kb_add(x_fold_upper,		NULL, CTRL('['), 'U', 0);
	kb_add(x_fold_upper,		NULL, CTRL('['), 'u', 0);
	kb_add(x_literal,		NULL, CTRL('V'), 0);
	kb_add(x_literal,		NULL, CTRL('^'), 0);
	kb_add(x_yank,			NULL, CTRL('Y'), 0);
	kb_add(x_meta_yank,		NULL, CTRL('['), 'y', 0);
	/* man page ends here */

	/* arrow keys */
	kb_add(x_prev_com,		NULL, CTRL('['), '[', 'A', 0); /* up */
	kb_add(x_next_com,		NULL, CTRL('['), '[', 'B', 0); /* down */
	kb_add(x_mv_forw,		NULL, CTRL('['), '[', 'C', 0); /* right */
	kb_add(x_mv_back,		NULL, CTRL('['), '[', 'D', 0); /* left */
	kb_add(x_prev_com,		NULL, CTRL('['), 'O', 'A', 0); /* up */
	kb_add(x_next_com,		NULL, CTRL('['), 'O', 'B', 0); /* down */
	kb_add(x_mv_forw,		NULL, CTRL('['), 'O', 'C', 0); /* right */
	kb_add(x_mv_back,		NULL, CTRL('['), 'O', 'D', 0); /* left */

	/* more navigation keys */
	kb_add(x_mv_begin,		NULL, CTRL('['), '[', 'H', 0); /* home */
	kb_add(x_mv_end,		NULL, CTRL('['), '[', 'F', 0); /* end */
	kb_add(x_mv_begin,		NULL, CTRL('['), 'O', 'H', 0); /* home */
	kb_add(x_mv_end,		NULL, CTRL('['), 'O', 'F', 0); /* end */

	/* can't be bound */
	kb_add(x_set_arg,		NULL, CTRL('['), '0', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '1', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '2', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '3', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '4', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '5', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '6', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '7', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '8', 0);
	kb_add(x_set_arg,		NULL, CTRL('['), '9', 0);

	/* ctrl arrow keys */
	kb_add(x_mv_end,		NULL, CTRL('['), '[', '1', ';', '5', 'A', 0); /* ctrl up */
	kb_add(x_mv_begin,		NULL, CTRL('['), '[', '1', ';', '5', 'B', 0); /* ctrl down */
	kb_add(x_mv_fword,		NULL, CTRL('['), '[', '1', ';', '5', 'C', 0); /* ctrl right */
	kb_add(x_mv_bword,		NULL, CTRL('['), '[', '1', ';', '5', 'D', 0); /* ctrl left */
}

void
x_emacs_keys(X_chars *ec)
{
	x_bind_quiet = 1;
	if (ec->erase >= 0) {
		kb_add(x_del_back, NULL, ec->erase, 0);
		kb_add(x_del_bword, NULL, CTRL('['), ec->erase, 0);
	}
	if (ec->kill >= 0)
		kb_add(x_del_line, NULL, ec->kill, 0);
	if (ec->werase >= 0)
		kb_add(x_del_bword, NULL, ec->werase, 0);
	if (ec->intr >= 0)
		kb_add(x_abort, NULL, ec->intr, 0);
	if (ec->quit >= 0)
		kb_add(x_noop, NULL, ec->quit, 0);
	x_bind_quiet = 0;
}

static int
x_set_mark(int c)
{
	xmp = xcp;
	return KSTD;
}

static int
x_kill_region(int c)
{
	int	rsize;
	char	*xr;

	if (xmp == NULL) {
		x_e_putc('\a');
		return KSTD;
	}
	if (xmp > xcp) {
		rsize = xmp - xcp;
		xr = xcp;
	} else {
		rsize = xcp - xmp;
		xr = xmp;
	}
	x_goto(xr);
	x_delete(rsize, true);
	xmp = xr;
	return KSTD;
}

static int
x_xchg_point_mark(int c)
{
	char	*tmp;

	if (xmp == NULL) {
		x_e_putc('\a');
		return KSTD;
	}
	tmp = xmp;
	xmp = xcp;
	x_goto( tmp );
	return KSTD;
}

static int
x_version(int c)
{
	char *o_xbuf = xbuf, *o_xend = xend;
	char *o_xep = xep, *o_xcp = xcp;

	xbuf = xcp = (char *) ksh_version + 4;
	xend = xep = (char *) ksh_version + 4 + strlen(ksh_version + 4);
	x_redraw(1);
	x_flush();

	c = x_e_getc(0);
	xbuf = o_xbuf;
	xend = o_xend;
	xep = o_xep;
	xcp = o_xcp;
	x_redraw(1);

	if (c < 0)
		return KSTD;
	/* This is what at&t ksh seems to do...  Very bizarre */
	if (c != ' ')
		x_e_ungetc(c);

	return KSTD;
}

static int
x_noop(int c)
{
	return KSTD;
}

/*
 *	File/command name completion routines
 */

static int
x_comp_comm(int c)
{
	do_complete(XCF_COMMAND, CT_COMPLETE);
	return KSTD;
}
static int
x_list_comm(int c)
{
	do_complete(XCF_COMMAND, CT_LIST);
	return KSTD;
}
static int
x_complete(int c)
{
	do_complete(XCF_COMMAND_FILE, CT_COMPLETE);
	return KSTD;
}
static int
x_enumerate(int c)
{
	do_complete(XCF_COMMAND_FILE, CT_LIST);
	return KSTD;
}
static int
x_comp_file(int c)
{
	do_complete(XCF_FILE, CT_COMPLETE);
	return KSTD;
}
static int
x_list_file(int c)
{
	do_complete(XCF_FILE, CT_LIST);
	return KSTD;
}
static int
x_comp_list(int c)
{
	do_complete(XCF_COMMAND_FILE, CT_COMPLIST);
	return KSTD;
}
static int
x_expand(int c)
{
	char **words;
	int nwords = 0;
	int start, end;
	int is_command;
	int i;

	nwords = x_cf_glob(XCF_FILE, xbuf, xep - xbuf, xcp - xbuf,
	    &start, &end, &words, &is_command);

	if (nwords == 0) {
		x_e_putc('\a');
		return KSTD;
	}

	x_goto(xbuf + start);
	x_delete(end - start, false);
	for (i = 0; i < nwords;) {
		if (x_escape(words[i], strlen(words[i]), x_emacs_putbuf) < 0 ||
		    (++i < nwords && x_ins(space) < 0)) {
			x_e_putc('\a');
			return KSTD;
		}
	}

	return KSTD;
}

static void
do_complete(int flags,	/* XCF_{COMMAND,FILE,COMMAND_FILE} */
    Comp_type type)
{
	char **words, *cp;
	int nwords;
	int start, end, nlen, olen;
	int is_command;
	int completed = 0;

	nwords = x_cf_glob(flags, xbuf, xep - xbuf, xcp - xbuf,
	    &start, &end, &words, &is_command);
	/* no match */
	if (nwords == 0) {
		x_e_putc('\a');
		return;
	}

	if (type == CT_LIST) {
		cp = xcp;
		x_goto(xep);
		xcp = cp;
		x_print_expansions(nwords, words, is_command);
		x_redraw(0);
		x_free_words(nwords, words);
		return;
	}

	olen = end - start;
	nlen = x_longest_prefix(nwords, words);
	/* complete */
	if (nwords == 1 || nlen > olen) {
		x_goto(xbuf + start);
		x_delete(olen, false);
		x_escape(words[0], nlen, x_emacs_putbuf);
		completed = 1;
		x_redraw(1);
	}
	/* add space if single non-dir match */
	if (nwords == 1 && words[0][nlen - 1] != '/') {
		x_ins(space);
		completed = 1;
	}

	if (type == CT_COMPLIST && !completed) {
		cp = xcp;
		x_goto(xep);
		xcp = cp;
		x_print_expansions(nwords, words, is_command);
		completed = 1;
		x_redraw(0);
	}

	x_free_words(nwords, words);
}

static int unget_char = -1;

static void
x_e_ungetc(int c)
{
	unget_char = c;
}

static int
x_e_getc(int redr)
{
	int c;

	if (unget_char >= 0) {
		c = unget_char;
		unget_char = -1;
		return (c);
	} else if (macro_args) {
		c = *macro_args++;
		if (c)
			return (c);
		macro_args = NULL;
	}

	while (got_sigwinch || ((c = x_getc()) < 0 && errno == EINTR)) {
		if (redr) {
			x_redraw(1);
			x_flush();
		}
	}

	return (c);
}

static void
x_e_putc(int c)
{
	x_putc(c);
	switch (c) {
	case '\a':
		break;
	case '\r':
	case '\n':
		x_col = 0;
		break;
	default:
		x_col++;
		if (x_col % x_cols == 0)
			x_puts("\r\n");
		break;
	}
}

#ifdef DEBUG
static int
x_debug_info(int c)
{
	x_flush();
	shellf("\nksh debug:\n");
	shellf("\tx_col == %d,\t\tx_cols == %d\n", x_col, x_cols);
	shellf("\txcp == 0x%lx,\txep == 0x%lx\n", (long) xcp, (long) xep);
	shellf("\txbuf == 0x%lx\n", (long) xbuf);
	shellf(newline);
	x_redraw(0);
	return 0;
}
#endif

static void
x_e_puts(const char *s)
{
	while (*s)
		x_e_putc(*s++);
}

/* NAME:
 *      x_set_arg - set an arg value for next function
 *
 * DESCRIPTION:
 *      This is a simple implementation of M-[0-9].
 *
 * RETURN VALUE:
 *      KSTD
 */

static int
x_set_arg(int c)
{
	int n = 0;
	int first = 1;

	for (; c >= 0 && isdigit(c); c = x_e_getc(0), first = 0)
		n = n * 10 + (c - '0');
	if (c < 0 || first) {
		x_e_putc('\a');
		x_arg = 1;
		x_arg_defaulted = 1;
	} else {
		x_e_ungetc(c);
		x_arg = n;
		x_arg_defaulted = 0;
		x_arg_set = 1;
	}
	return KSTD;
}


/* Comment or uncomment the current line. */
static int
x_comment(int c)
{
	int len = xep - xbuf;
	int ret = x_do_comment(xbuf, xend - xbuf, &len);

	if (ret < 0)
		x_e_putc('\a');
	else {
		xep = xbuf + len;
		*xep = '\0';
		xcp = xbuf;
		x_redraw(1);
		if (ret > 0)
			return x_newline('\n');
	}
	return KSTD;
}


/* NAME:
 *      x_prev_histword - recover word from prev command
 *
 * DESCRIPTION:
 *      This function recovers the last word from the previous
 *      command and inserts it into the current edit line.  If a
 *      numeric arg is supplied then the n'th word from the
 *      start of the previous command is used.
 *
 *      Bound to M-.
 *
 * RETURN VALUE:
 *      KSTD
 */

static int
x_prev_histword(int c)
{
	char *rcp;
	char *cp;

	cp = *histptr;
	if (!cp)
		x_e_putc('\a');
	else if (x_arg_defaulted) {
		rcp = &cp[strlen(cp) - 1];
		/*
		 * ignore white-space after the last word
		 */
		while (rcp > cp && is_cfs(*rcp))
			rcp--;
		while (rcp > cp && !is_cfs(*rcp))
			rcp--;
		if (is_cfs(*rcp))
			rcp++;
		x_ins(rcp);
	} else {
		int c;

		rcp = cp;
		/*
		 * ignore white-space at start of line
		 */
		while (*rcp && is_cfs(*rcp))
			rcp++;
		while (x_arg-- > 1) {
			while (*rcp && !is_cfs(*rcp))
				rcp++;
			while (*rcp && is_cfs(*rcp))
				rcp++;
		}
		cp = rcp;
		while (*rcp && !is_cfs(*rcp))
			rcp++;
		c = *rcp;
		*rcp = '\0';
		x_ins(cp);
		*rcp = c;
	}
	return KSTD;
}

/* Uppercase N(1) words */
static int
x_fold_upper(int c)
{
	return x_fold_case('U');
}

/* Lowercase N(1) words */
static int
x_fold_lower(int c)
{
	return x_fold_case('L');
}

/* Lowercase N(1) words */
static int
x_fold_capitalize(int c)
{
	return x_fold_case('C');
}

/* NAME:
 *      x_fold_case - convert word to UPPER/lower/Capital case
 *
 * DESCRIPTION:
 *      This function is used to implement M-U,M-u,M-L,M-l,M-C and M-c
 *      to UPPER case, lower case or Capitalize words.
 *
 * RETURN VALUE:
 *      None
 */

static int
x_fold_case(int c)
{
	char *cp = xcp;

	if (cp == xep) {
		x_e_putc('\a');
		return KSTD;
	}
	while (x_arg--) {
		/*
		 * first skip over any white-space
		 */
		while (cp != xep && is_mfs(*cp))
			cp++;
		/*
		 * do the first char on its own since it may be
		 * a different action than for the rest.
		 */
		if (cp != xep) {
			if (c == 'L') {		/* lowercase */
				if (isupper((unsigned char)*cp))
					*cp = tolower((unsigned char)*cp);
			} else {		/* uppercase, capitalize */
				if (islower((unsigned char)*cp))
					*cp = toupper((unsigned char)*cp);
			}
			cp++;
		}
		/*
		 * now for the rest of the word
		 */
		while (cp != xep && !is_mfs(*cp)) {
			if (c == 'U') {		/* uppercase */
				if (islower((unsigned char)*cp))
					*cp = toupper((unsigned char)*cp);
			} else {		/* lowercase, capitalize */
				if (isupper((unsigned char)*cp))
					*cp = tolower((unsigned char)*cp);
			}
			cp++;
		}
	}
	x_goto(cp);
	return KSTD;
}

#endif /* EMACS */
