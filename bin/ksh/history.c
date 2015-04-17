/*	$OpenBSD: history.c,v 1.40 2014/11/20 15:22:39 tedu Exp $	*/

#include "sh.h"

#ifdef HISTORY
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>

static void	writehistfile(void);
static FILE	*history_open(void);
static int	history_load(Source *);
static void	history_close(void);

static int	hist_execute(char *);
static int	hist_replace(char **, const char *, const char *, int);
static char	**hist_get(const char *, int, int);
static char	**hist_get_oldest(void);
static void	histbackup(void);

static FILE	*histfd;
static char	**current;	/* current position in history[] */
static char	*hname;		/* current name of history file */
static int	hstarted;	/* set after hist_init() called */
static Source	*hist_source;
static uint32_t	line_co;
static int	real_fs = 0;
static uint32_t	ro_fs = 0;
static int	lockfd = -1;
static char	*lname;

static struct stat last_sb;

int
c_fc(char **wp)
{
	struct shf	*shf;
	struct temp	*tf = NULL;
	char		*p, *editor = NULL;
	int		gflag = 0, lflag = 0, nflag = 0, sflag = 0, rflag = 0;
	int		optc;
	char		*first = NULL, *last = NULL;
	char		**hfirst, **hlast, **hp;

	if (!Flag(FTALKING_I)) {
		bi_errorf("history functions not available");
		return 1;
	}

	while ((optc = ksh_getopt(wp, &builtin_opt,
	    "e:glnrs0,1,2,3,4,5,6,7,8,9,")) != -1)
		switch (optc) {
		case 'e':
			p = builtin_opt.optarg;
			if (strcmp(p, "-") == 0)
				sflag++;
			else {
				size_t len = strlen(p) + 4;
				editor = str_nsave(p, len, ATEMP);
				strlcat(editor, " $_", len);
			}
			break;
		case 'g': /* non-at&t ksh */
			gflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 's':	/* posix version of -e - */
			sflag++;
			break;
		  /* kludge city - accept -num as -- -num (kind of) */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			p = shf_smprintf("-%c%s",
					optc, builtin_opt.optarg);
			if (!first)
				first = p;
			else if (!last)
				last = p;
			else {
				bi_errorf("too many arguments");
				return 1;
			}
			break;
		case '?':
			return 1;
		}
	wp += builtin_opt.optind;

	/* Substitute and execute command */
	if (sflag) {
		char *pat = NULL, *rep = NULL;

		if (editor || lflag || nflag || rflag) {
			bi_errorf("can't use -e, -l, -n, -r with -s (-e -)");
			return 1;
		}

		/* Check for pattern replacement argument */
		if (*wp && **wp && (p = strchr(*wp + 1, '='))) {
			pat = str_save(*wp, ATEMP);
			p = pat + (p - *wp);
			*p++ = '\0';
			rep = p;
			wp++;
		}
		/* Check for search prefix */
		if (!first && (first = *wp))
			wp++;
		if (last || *wp) {
			bi_errorf("too many arguments");
			return 1;
		}

		hp = first ? hist_get(first, false, false) :
		    hist_get_newest(false);
		if (!hp)
			return 1;
		return hist_replace(hp, pat, rep, gflag);
	}

	if (editor && (lflag || nflag)) {
		bi_errorf("can't use -l, -n with -e");
		return 1;
	}

	if (!first && (first = *wp))
		wp++;
	if (!last && (last = *wp))
		wp++;
	if (*wp) {
		bi_errorf("too many arguments");
		return 1;
	}
	if (!first) {
		hfirst = lflag ? hist_get("-16", true, true) :
		    hist_get_newest(false);
		if (!hfirst)
			return 1;
		/* can't fail if hfirst didn't fail */
		hlast = hist_get_newest(false);
	} else {
		/* POSIX says not an error if first/last out of bounds
		 * when range is specified; at&t ksh and pdksh allow out of
		 * bounds for -l as well.
		 */
		hfirst = hist_get(first, (lflag || last) ? true : false,
		    lflag ? true : false);
		if (!hfirst)
			return 1;
		hlast = last ? hist_get(last, true, lflag ? true : false) :
		    (lflag ? hist_get_newest(false) : hfirst);
		if (!hlast)
			return 1;
	}
	if (hfirst > hlast) {
		char **temp;

		temp = hfirst; hfirst = hlast; hlast = temp;
		rflag = !rflag; /* POSIX */
	}

	/* List history */
	if (lflag) {
		char *s, *t;
		const char *nfmt = nflag ? "\t" : "%d\t";

		for (hp = rflag ? hlast : hfirst;
		    hp >= hfirst && hp <= hlast; hp += rflag ? -1 : 1) {
			shf_fprintf(shl_stdout, nfmt,
			    hist_source->line - (int) (histptr - hp));
			/* print multi-line commands correctly */
			for (s = *hp; (t = strchr(s, '\n')); s = t)
				shf_fprintf(shl_stdout, "%.*s\t", ++t - s, s);
			shf_fprintf(shl_stdout, "%s\n", s);
		}
		shf_flush(shl_stdout);
		return 0;
	}

	/* Run editor on selected lines, then run resulting commands */

	tf = maketemp(ATEMP, TT_HIST_EDIT, &e->temps);
	if (!(shf = tf->shf)) {
		bi_errorf("cannot create temp file %s - %s",
		    tf->name, strerror(errno));
		return 1;
	}
	for (hp = rflag ? hlast : hfirst;
	    hp >= hfirst && hp <= hlast; hp += rflag ? -1 : 1)
		shf_fprintf(shf, "%s\n", *hp);
	if (shf_close(shf) == EOF) {
		bi_errorf("error writing temporary file - %s", strerror(errno));
		return 1;
	}

	/* Ignore setstr errors here (arbitrary) */
	setstr(local("_", false), tf->name, KSH_RETURN_ERROR);

	/* XXX: source should not get trashed by this.. */
	{
		Source *sold = source;
		int ret;

		ret = command(editor ? editor : "${FCEDIT:-/bin/ed} $_", 0);
		source = sold;
		if (ret)
			return ret;
	}

	{
		struct stat statb;
		XString xs;
		char *xp;
		int n;

		if (!(shf = shf_open(tf->name, O_RDONLY, 0, 0))) {
			bi_errorf("cannot open temp file %s", tf->name);
			return 1;
		}

		n = fstat(shf_fileno(shf), &statb) < 0 ? 128 :
		    statb.st_size + 1;
		Xinit(xs, xp, n, hist_source->areap);
		while ((n = shf_read(xp, Xnleft(xs, xp), shf)) > 0) {
			xp += n;
			if (Xnleft(xs, xp) <= 0)
				XcheckN(xs, xp, Xlength(xs, xp));
		}
		if (n < 0) {
			bi_errorf("error reading temp file %s - %s",
			    tf->name, strerror(shf_errno(shf)));
			shf_close(shf);
			return 1;
		}
		shf_close(shf);
		*xp = '\0';
		strip_nuls(Xstring(xs, xp), Xlength(xs, xp));
		return hist_execute(Xstring(xs, xp));
	}
}

/* Save cmd in history, execute cmd (cmd gets trashed) */
static int
hist_execute(char *cmd)
{
	Source		*sold;
	int		ret;
	char		*p, *q;

	histbackup();

	for (p = cmd; p; p = q) {
		if ((q = strchr(p, '\n'))) {
			*q++ = '\0'; /* kill the newline */
			if (!*q) /* ignore trailing newline */
				q = NULL;
		}
		histsave(++(hist_source->line), p, 1);

		shellf("%s\n", p); /* POSIX doesn't say this is done... */
		if ((p = q)) /* restore \n (trailing \n not restored) */
			q[-1] = '\n';
	}

	/* Commands are executed here instead of pushing them onto the
	 * input 'cause posix says the redirection and variable assignments
	 * in
	 *	X=y fc -e - 42 2> /dev/null
	 * are to effect the repeated commands environment.
	 */
	/* XXX: source should not get trashed by this.. */
	sold = source;
	ret = command(cmd, 0);
	source = sold;
	return ret;
}

static int
hist_replace(char **hp, const char *pat, const char *rep, int global)
{
	char		*line;

	if (!pat)
		line = str_save(*hp, ATEMP);
	else {
		char *s, *s1;
		int pat_len = strlen(pat);
		int rep_len = strlen(rep);
		int len;
		XString xs;
		char *xp;
		int any_subst = 0;

		Xinit(xs, xp, 128, ATEMP);
		for (s = *hp; (s1 = strstr(s, pat)) && (!any_subst || global);
		    s = s1 + pat_len) {
			any_subst = 1;
			len = s1 - s;
			XcheckN(xs, xp, len + rep_len);
			memcpy(xp, s, len);		/* first part */
			xp += len;
			memcpy(xp, rep, rep_len);	/* replacement */
			xp += rep_len;
		}
		if (!any_subst) {
			bi_errorf("substitution failed");
			return 1;
		}
		len = strlen(s) + 1;
		XcheckN(xs, xp, len);
		memcpy(xp, s, len);
		xp += len;
		line = Xclose(xs, xp);
	}
	return hist_execute(line);
}

/*
 * get pointer to history given pattern
 * pattern is a number or string
 */
static char **
hist_get(const char *str, int approx, int allow_cur)
{
	char		**hp = NULL;
	int		n;

	if (getn(str, &n)) {
		hp = histptr + (n < 0 ? n : (n - hist_source->line));
		if ((long)hp < (long)history) {
			if (approx)
				hp = hist_get_oldest();
			else {
				bi_errorf("%s: not in history", str);
				hp = NULL;
			}
		} else if (hp > histptr) {
			if (approx)
				hp = hist_get_newest(allow_cur);
			else {
				bi_errorf("%s: not in history", str);
				hp = NULL;
			}
		} else if (!allow_cur && hp == histptr) {
			bi_errorf("%s: invalid range", str);
			hp = NULL;
		}
	} else {
		int anchored = *str == '?' ? (++str, 0) : 1;

		/* the -1 is to avoid the current fc command */
		n = findhist(histptr - history - 1, 0, str, anchored);
		if (n < 0) {
			bi_errorf("%s: not in history", str);
			hp = NULL;
		} else
			hp = &history[n];
	}
	return hp;
}

/* Return a pointer to the newest command in the history */
char **
hist_get_newest(int allow_cur)
{
	if (histptr < history || (!allow_cur && histptr == history)) {
		bi_errorf("no history (yet)");
		return NULL;
	}
	if (allow_cur)
		return histptr;
	return histptr - 1;
}

/* Return a pointer to the oldest command in the history */
static char **
hist_get_oldest(void)
{
	if (histptr <= history) {
		bi_errorf("no history (yet)");
		return NULL;
	}
	return history;
}

/******************************/
/* Back up over last histsave */
/******************************/
static void
histbackup(void)
{
	static int	last_line = -1;

	if (histptr >= history && last_line != hist_source->line) {
		hist_source->line--;
		afree((void*)*histptr, APERM);
		histptr--;
		last_line = hist_source->line;
	}
}

/*
 * Return the current position.
 */
char **
histpos(void)
{
	return current;
}

int
histnum(int n)
{
	int		last = histptr - history;

	if (n < 0 || n >= last) {
		current = histptr;
		return last;
	} else {
		current = &history[n];
		return n;
	}
}

/*
 * This will become unnecessary if hist_get is modified to allow
 * searching from positions other than the end, and in either
 * direction.
 */
int
findhist(int start, int fwd, const char *str, int anchored)
{
	char		**hp;
	int		maxhist = histptr - history;
	int		incr = fwd ? 1 : -1;
	int		len = strlen(str);

	if (start < 0 || start >= maxhist)
		start = maxhist;

	hp = &history[start];
	for (; hp >= history && hp <= histptr; hp += incr)
		if ((anchored && strncmp(*hp, str, len) == 0) ||
		    (!anchored && strstr(*hp, str)))
			return hp - history;

	return -1;
}

int
findhistrel(const char *str)
{
	int		maxhist = histptr - history;
	int		start = maxhist - 1;
	int		rec = atoi(str);

	if (rec == 0)
		return -1;
	if (rec > 0) {
		if (rec > maxhist)
			return -1;
		return rec - 1;
	}
	if (rec > maxhist)
		return -1;
	return start + rec + 1;
}

/*
 *	set history
 *	this means reallocating the dataspace
 */
void
sethistsize(int n)
{
	if (n > 0 && n != histsize) {
		int cursize = histptr - history;

		/* save most recent history */
		if (n < cursize) {
			memmove(history, histptr - n, n * sizeof(char *));
			cursize = n;
		}

		history = aresizearray(history, n, sizeof(char *), APERM);

		histsize = n;
		histptr = history + cursize;
	}
}

/*
 *	set history file
 *	This can mean reloading/resetting/starting history file
 *	maintenance
 */
void
sethistfile(const char *name)
{
	/* if not started then nothing to do */
	if (hstarted == 0)
		return;

	/* if the name is the same as the name we have */
	if (hname && strcmp(hname, name) == 0)
		return;
	/*
	 * its a new name - possibly
	 */
	if (hname) {
		afree(hname, APERM);
		hname = NULL;
		/* let's reset the history */
		histptr = history - 1;
		hist_source->line = 0;
	}

	if (histfd)
		history_close();

	hist_init(hist_source);
}

/*
 *	initialise the history vector
 */
void
init_histvec(void)
{
	if (history == NULL) {
		histsize = HISTORYSIZE;
		history = acalloc(histsize, sizeof(char *), APERM);
		histptr = history - 1;
	}
}

static int
history_lock(void)
{
	int			tries;

	if (real_fs) {
		while (flock(fileno(histfd), LOCK_EX) != 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			else
				break;
		}
	} else {
		if (lockfd != -1) {
			/* should not happen */
			bi_errorf("recursive lock");
			close(lockfd);
			lockfd = -1;
		}
		for (tries = 0; ro_fs == 0 && tries < 30; tries++) {
			if ((lockfd = open(lname, O_WRONLY | O_CREAT |
			    O_EXLOCK | O_EXCL, 0600)) != -1)
				return (0);
			usleep(100000); /* 100mS */
		}
		/* locking failed */
		return (1);
	}

	return (0);
}

static void
history_unlock(void)
{
	if (real_fs) {
		while (flock(fileno(histfd), LOCK_UN) != 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			else
				break;
		}
	} else {
		if (lockfd == -1) {
			/* should not happen */
			bi_errorf("invalid lock");
			return;
		}
		if (unlink(lname) == -1) {
			/* should not happen */
			bi_errorf("can't unlink lock file");
			return;
		}
		close(lockfd);
		lockfd = -1;
	}
}

/*
 * save command in history
 */
void
histsave(int lno, const char *cmd, int dowrite)
{
	char		**hp;
	char		*c, *cp;
	struct stat	sb;
	int		gotlock = 0;

	if (dowrite && histfd) {
		if (history_lock() == 0) {
			gotlock = 1;
			if (fstat(fileno(histfd), &sb) != -1) {
				if (timespeccmp(&sb.st_mtim,
				    &last_sb.st_mtim, ==))
					; /* file is unchanged */
				else {
					/* reset history */
					histptr = history - 1;
					hist_source->line = 0;
					history_load(hist_source);
				}
			}
		}
	}

	c = str_save(cmd, APERM);
	if ((cp = strchr(c, '\n')) != NULL)
		*cp = '\0';

	hp = histptr;
	if (++hp >= history + histsize) { /* remove oldest command */
		afree((void*)*history, APERM);
		for (hp = history; hp < history + histsize - 1; hp++)
			hp[0] = hp[1];
	}
	*hp = c;
	histptr = hp;

	if (dowrite && histfd) {
		if (gotlock) {
			/* append to file */
			if (fseeko(histfd, 0, SEEK_END) == 0) {
				fprintf(histfd, "%s", cmd);
				fflush(histfd);
				fstat(fileno(histfd), &last_sb);
				line_co++;
				writehistfile();
			}
			history_unlock();
		}
	}
}

static FILE *
history_open(void)
{
	int		fd, fddup, flags;
	FILE		*f = NULL;
	uint8_t		old[2];

	if (real_fs == 0) {
		if (history_lock()) {
			bi_errorf("initial locking failed, history file won't "
			    "be used");
			return (NULL);
		}
		flags = O_RDWR | O_CREAT;
	} else
		flags = O_RDWR | O_CREAT | O_EXLOCK;

	if ((fd = open(hname, flags, 0600)) == -1)
		return (NULL);

	fddup = savefd(fd);
	if (fddup != fd)
		close(fd);

	f = fdopen(fddup, "r+");
	if (f == NULL) {
		close(fddup);
		goto bad;
	}

	/* check for old format */
	if (fread(old, sizeof old, 1, f) == 1) {
		if (old[0] == 0xab && old[1] == 0xcd) {
			bi_errorf("binary format history file detected, "
			    "history file won't be used");
			goto bad;
		}
		rewind(f);
	}

	fstat(fileno(f), &last_sb);

	return (f);
bad:
	if (f)
		fclose(f);

	return (NULL);
}

static void
history_close(void)
{
	if (histfd) {
		fflush(histfd);
		fclose(histfd);
		histfd = NULL;
	}
}

static int
history_load(Source *s)
{
	char		*p, *lbuf;
	size_t		len;

	rewind(histfd);

	/* just read it all; will auto resize history upon next command */
	for (line_co = 1; ; line_co++) {
		p = fgetln(histfd, &len);
		if (p == NULL || feof(histfd) || ferror(histfd))
			break;
		/* really shouldn't happen */
		if (len == 0)
			continue;

		lbuf = NULL;
		if (p[len - 1] == '\n')
			p[len - 1] = '\0';
		else {
			/* EOF without EOL, copy and add the NUL */
			if ((lbuf = malloc(len + 1)) == NULL) {
				bi_errorf("history_load: out of memory");
				return (1);
			}
			memcpy(lbuf, p, len);
			lbuf[len] = '\0';
			p = lbuf;
		}

		s->line = line_co;
		s->cmd_offset = line_co;
		histsave(line_co, p, 0);

		if (lbuf)
			free(lbuf);
	}

	writehistfile();

	return (0);
}

void
hist_init(Source *s)
{
	struct statfs	sf;

	if (Flag(FTALKING) == 0)
		return;

	hstarted = 1;

	hist_source = s;

	hname = str_val(global("HISTFILE"));
	if (hname == NULL || strlen(hname) == 0)
		return;
	hname = str_save(hname, APERM);
	if (statfs(hname, &sf) == 0) {
		if (!strcmp(sf.f_fstypename, "ffs"))
			real_fs = 1;
		ro_fs = sf.f_flags & MNT_RDONLY;
	}
	if (real_fs == 0)
		asprintf(&lname, "%s.lock", hname);
	histfd = history_open();
	if (histfd == NULL)
		return;

	history_load(s);

	history_unlock();
}

static void
writehistfile(void)
{
	int		i;
	char		*cmd;

	/* see if file has grown over 125% */
	if (line_co < histsize + (histsize / 4))
		return;

	if (ftruncate(fileno(histfd), 0) == -1)
		return;

	/* rewrite the whole kaboodle */
	rewind(histfd);
	for (i = 0; i < histsize; i++) {
		cmd = history[i];
		if (cmd == NULL)
			break;
		if (fprintf(histfd, "%s\n", cmd) == -1)
			return;
	}

	line_co = histsize;

	fflush(histfd);
	fstat(fileno(histfd), &last_sb);
}

void
hist_finish(void)
{
	history_close();
}
#else /* HISTORY */

/* No history to be compiled in: dummy routines to avoid lots more ifdefs */
void
init_histvec(void)
{
}
void
hist_init(Source *s)
{
}
void
hist_finish(void)
{
}
void
histsave(int lno, const char *cmd, int dowrite)
{
	errorf("history not enabled");
}
#endif /* HISTORY */
