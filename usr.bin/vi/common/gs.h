/*	$OpenBSD: gs.h,v 1.14 2015/03/27 04:11:25 brynet Exp $	*/

/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 *
 *	@(#)gs.h	10.34 (Berkeley) 9/24/96
 */

#define	TEMPORARY_FILE_STRING	"/tmp"	/* Default temporary file name. */

/*
 * File reference structure (FREF).  The structure contains the name of the
 * file, along with the information that follows the name.
 *
 * !!!
 * The read-only bit follows the file name, not the file itself.
 */
struct _fref {
	TAILQ_ENTRY(_fref) q;		/* Linked list of file references. */
	char	*name;			/* File name. */
	char	*tname;			/* Backing temporary file name. */

	recno_t	 lno;			/* 1-N: file cursor line. */
	size_t	 cno;			/* 0-N: file cursor column. */

#define	FR_CURSORSET	0x0001		/* If lno/cno values valid. */
#define	FR_DONTDELETE	0x0002		/* Don't delete the temporary file. */
#define	FR_EXNAMED	0x0004		/* Read/write renamed the file. */
#define	FR_NAMECHANGE	0x0008		/* If the name changed. */
#define	FR_NEWFILE	0x0010		/* File doesn't really exist yet. */
#define	FR_RECOVER	0x0020		/* File is being recovered. */
#define	FR_TMPEXIT	0x0040		/* Modified temporary file, no exit. */
#define	FR_TMPFILE	0x0080		/* If file has no name. */
#define	FR_UNLOCKED	0x0100		/* File couldn't be locked. */
	u_int16_t flags;
};

/* Action arguments to cl_ex_adjust(). */
typedef enum { EX_TERM_CE, EX_TERM_SCROLL } exadj_t;

/* Screen attribute arguments to cl_attr(). */
typedef enum { SA_ALTERNATE, SA_INVERSE } scr_attr_t;

/* Key type arguments to cl_keyval(). */
typedef enum { KEY_VEOF, KEY_VERASE, KEY_VKILL, KEY_VWERASE } scr_keyval_t;

/*
 * GS:
 *
 * Structure that describes global state of the running program.
 */
struct _gs {
	char	*progname;		/* Programe name. */

	int	 id;			/* Last allocated screen id. */
	TAILQ_HEAD(_dqh, _scr) dq;	/* Displayed screens. */
	TAILQ_HEAD(_hqh, _scr) hq;	/* Hidden screens. */

	SCR	*ccl_sp;		/* Colon command-line screen. */

	void	*cl_private;		/* Curses support private area. */

					/* File references. */
	TAILQ_HEAD(_frefh, _fref) frefq;

#define	GO_COLUMNS	0		/* Global options: columns. */
#define	GO_LINES	1		/* Global options: lines. */
#define	GO_SECURE	2		/* Global options: secure. */
#define	GO_TERM		3		/* Global options: terminal type. */
	OPTION	 opts[GO_TERM + 1];

	DB	*msg;			/* Message catalog DB. */
	MSGH	 msgq;			/* User message list. */
#define	DEFAULT_NOPRINT	'\1'		/* Emergency non-printable character. */
	CHAR_T	 noprint;		/* Cached, unprintable character. */

	char	*tmp_bp;		/* Temporary buffer. */
	size_t	 tmp_blen;		/* Temporary buffer size. */

	/*
	 * Ex command structures (EXCMD).  Defined here because ex commands
	 * exist outside of any particular screen or file.
	 */
#define	EXCMD_RUNNING(gp)	(LIST_FIRST(&(gp)->ecq)->clen != 0)
	LIST_HEAD(_excmdh, _excmd) ecq;	/* Ex command linked list. */
	EXCMD	 excmd;			/* Default ex command structure. */
	char	 *if_name;		/* Current associated file. */
	recno_t	  if_lno;		/* Current associated line number. */

	char	*c_option;		/* Ex initial, command-line command. */

#ifdef DEBUG
	FILE	*tracefp;		/* Trace file pointer. */
#endif

	EVENT	*i_event;		/* Array of input events. */
	size_t	 i_nelem;		/* Number of array elements. */
	size_t	 i_cnt;			/* Count of events. */
	size_t	 i_next;		/* Offset of next event. */

	CB	*dcbp;			/* Default cut buffer pointer. */
	CB	 dcb_store;		/* Default cut buffer storage. */
	LIST_HEAD(_cuth, _cb) cutq;	/* Linked list of cut buffers. */

#define	MAX_BIT_SEQ	128		/* Max + 1 fast check character. */
	LIST_HEAD(_seqh, _seq) seqq;	/* Linked list of maps, abbrevs. */
	bitstr_t bit_decl(seqb, MAX_BIT_SEQ);

#define	MAX_FAST_KEY	254		/* Max fast check character.*/
#define	KEY_LEN(sp, ch)							\
	((unsigned char)(ch) <= MAX_FAST_KEY ?				\
	    (sp)->gp->cname[(unsigned char)(ch)].len :			\
	    v_key_len((sp), (ch)))
#define	KEY_NAME(sp, ch)						\
	((unsigned char)(ch) <= MAX_FAST_KEY ?				\
	    (sp)->gp->cname[(unsigned char)(ch)].name :			\
	    v_key_name((sp), (ch)))
	struct {
		CHAR_T	 name[MAX_CHARACTER_COLUMNS + 1];
		u_int8_t len;
	} cname[MAX_FAST_KEY + 1];	/* Fast lookup table. */

#define	KEY_VAL(sp, ch)							\
	((unsigned char)(ch) <= MAX_FAST_KEY ? 				\
	    (sp)->gp->special_key[(unsigned char)(ch)] :		\
	    (unsigned char)(ch) > (sp)->gp->max_special ? 0 :		\
	    v_key_val((sp),(ch)))
	CHAR_T	 max_special;		/* Max special character. */
	u_char				/* Fast lookup table. */
	    special_key[MAX_FAST_KEY + 1];

/* Flags. */
#define	G_ABBREV	0x0001		/* If have abbreviations. */
#define	G_BELLSCHED	0x0002		/* Bell scheduled. */
#define	G_INTERRUPTED	0x0004		/* Interrupted. */
#define	G_RECOVER_SET	0x0008		/* Recover system initialized. */
#define	G_SCRIPTED	0x0010		/* Ex script session. */
#define	G_SCRWIN	0x0020		/* Scripting windows running. */
#define	G_SNAPSHOT	0x0040		/* Always snapshot files. */
#define	G_SRESTART	0x0080		/* Screen restarted. */
#define	G_TMP_INUSE	0x0100		/* Temporary buffer in use. */
	u_int32_t flags;
};
