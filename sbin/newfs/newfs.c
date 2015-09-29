/*	$OpenBSD: newfs.c,v 1.100 2015/09/29 03:19:24 guenther Exp $	*/
/*	$NetBSD: newfs.c,v 1.20 1996/05/16 07:13:03 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program.
 *
 * Copyright (c) 1983, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dkio.h>
#include <sys/disklabel.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <util.h>

#include "mntopts.h"
#include "pathnames.h"

struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_UPDATE,
	MOPT_FORCE,
	{ NULL },
};

void	fatal(const char *fmt, ...)
	    __attribute__((__format__ (printf, 1, 2)))
	    __attribute__((__nonnull__ (1)));
__dead void	usage(void);
void	mkfs(struct partition *, char *, int, int);
void	rewritelabel(char *, int, struct disklabel *);
u_short	dkcksum(struct disklabel *);

/*
 * The following two constants set the default block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	sectorsize <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define	DFL_FRAGSIZE	2048
#define	DFL_BLKSIZE	16384

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. The default is one indirect
 * block worth of data blocks.
 */
#define MAXBLKPG_FFS1(bsize)	((bsize) / sizeof(int32_t))
#define MAXBLKPG_FFS2(bsize)	((bsize) / sizeof(int64_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NFPI fragments, expecting this
 * to be far more than we will ever need.
 */
#define	NFPI		4

int	Nflag;			/* run without writing file system */
int	Oflag = 2;		/* 0 = 4.3BSD ffs, 1 = 4.4BSD ffs, 2 = ffs2 */
daddr_t	fssize;			/* file system size in 512-byte blocks */
long long	sectorsize;		/* bytes/sector */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	maxfrgspercg = INT_MAX;	/* maximum fragments per cylinder group */
int	minfree = MINFREE;	/* free space threshold */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	reqopt = -1;		/* opt preference has not been specified */
int	density;		/* number of bytes per inode */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	avgfilesize = AVFILESIZ;/* expected average file size */
int	avgfilesperdir = AFPDIR;/* expected number of files per directory */
int	mntflags = MNT_ASYNC;	/* flags to be passed to mount */
int	quiet = 0;		/* quiet flag */
caddr_t	membase;		/* start address of memory based filesystem */
char	*disktype;
int	unlabeled;

extern	char *__progname;
struct disklabel *getdisklabel(char *, int);

int
main(int argc, char *argv[])
{
	int ch;
	struct partition *pp;
	struct disklabel *lp;
	struct partition oldpartition;
	struct stat st;
	struct statfs *mp;
	struct rlimit rl;
	int fsi = -1, fso, len, n, maxpartitions;
	char *cp = NULL, *s1, *s2, *special, *opstring, *realdev;
	char *fstype = NULL;
	char **saveargv = argv;
	int ffsflag = 1;
	const char *errstr;
	long long fssize_input = 0;
	int fssize_usebytes = 0;
	u_int64_t nsecs;

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		fatal("insane maxpartitions value %d", maxpartitions);

	opstring = "NO:S:T:b:c:e:f:g:h:i:m:o:qs:t:";
	while ((ch = getopt(argc, argv, opstring)) != -1) {
		switch (ch) {
		case 'N':
			Nflag = 1;
			break;
		case 'O':
			Oflag = strtonum(optarg, 0, 2, &errstr);
			if (errstr)
				fatal("%s: invalid ffs version", optarg);
			break;
		case 'S':
			if (scan_scaled(optarg, &sectorsize) == -1 ||
			    sectorsize <= 0 || (sectorsize % DEV_BSIZE))
				fatal("sector size invalid: %s", optarg);
			break;
		case 'T':
			disktype = optarg;
			break;
		case 'b':
			bsize = strtonum(optarg, MINBSIZE, MAXBSIZE, &errstr);
			if (errstr)
				fatal("block size is %s: %s", errstr, optarg);
			break;
		case 'c':
			maxfrgspercg = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				fatal("fragments per cylinder group is %s: %s",
				    errstr, optarg);
			break;
		case 'e':
			maxbpg = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				fatal("blocks per file in a cylinder group is"
				    " %s: %s", errstr, optarg);
			break;
		case 'f':
			fsize = strtonum(optarg, MINBSIZE / MAXFRAG, MAXBSIZE,
			    &errstr);
			if (errstr)
				fatal("fragment size is %s: %s",
				    errstr, optarg);
			break;
		case 'g':
			avgfilesize = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				fatal("average file size is %s: %s",
				    errstr, optarg);
			break;
		case 'h':
			avgfilesperdir = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				fatal("average files per dir is %s: %s",
				    errstr, optarg);
			break;
		case 'i':
			density = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				fatal("bytes per inode is %s: %s",
				    errstr, optarg);
			break;
		case 'm':
			minfree = strtonum(optarg, 0, 99, &errstr);
			if (errstr)
				fatal("free space %% is %s: %s",
				    errstr, optarg);
			break;
		case 'o':
			if (strcmp(optarg, "space") == 0)
				reqopt = opt = FS_OPTSPACE;
			else if (strcmp(optarg, "time") == 0)
				reqopt = opt = FS_OPTTIME;
			else
				fatal("%s: unknown optimization preference: "
				    "use `space' or `time'.", optarg);
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			if (scan_scaled(optarg, &fssize_input) == -1 ||
			    fssize_input == 0)
				fatal("file system size invalid: %s", optarg);
			fssize_usebytes = 0;    /* in case of multiple -s */
			for (s1 = optarg; *s1 != '\0'; s1++)
				if (isalpha((unsigned char)*s1)) {
					fssize_usebytes = 1;
					break;
				}
			break;
		case 't':
			fstype = optarg;
			if (strcmp(fstype, "ffs"))
				ffsflag = 0;
			break;
		case '?':
		default:
			usage();
		}
		if (!ffsflag)
			break;
	}
	argc -= optind;
	argv += optind;

	if (ffsflag && argc != 1)
		usage();

	special = argv[0];

	char execname[MAXPATHLEN], name[MAXPATHLEN];

	if (fstype == NULL)
		fstype = readlabelfs(special, 0);
	if (fstype != NULL && strcmp(fstype, "ffs")) {
		snprintf(name, sizeof name, "newfs_%s", fstype);
		saveargv[0] = name;
		snprintf(execname, sizeof execname, "%s/newfs_%s", _PATH_SBIN,
		    fstype);
		(void)execv(execname, saveargv);
		snprintf(execname, sizeof execname, "%s/newfs_%s",
		    _PATH_USRSBIN, fstype);
		(void)execv(execname, saveargv);
		err(1, "%s not found", name);
	}

	if (Nflag) {
		fso = -1;
	} else {
		fso = opendev(special, O_WRONLY, 0, &realdev);
		if (fso < 0)
			fatal("%s: %s", special, strerror(errno));
		special = realdev;

		/* Bail if target special is mounted */
		n = getmntinfo(&mp, MNT_NOWAIT);
		if (n == 0)
			fatal("%s: getmntinfo: %s", special, strerror(errno));

		len = sizeof(_PATH_DEV) - 1;
		s1 = special;
		if (strncmp(_PATH_DEV, s1, len) == 0)
			s1 += len;

		while (--n >= 0) {
			s2 = mp->f_mntfromname;
			if (strncmp(_PATH_DEV, s2, len) == 0) {
				s2 += len - 1;
				*s2 = 'r';
			}
			if (strcmp(s1, s2) == 0 || strcmp(s1, &s2[1]) == 0)
				fatal("%s is mounted on %s",
				    special, mp->f_mntonname);
			++mp;
		}
	}
	fsi = opendev(special, O_RDONLY, 0, NULL);
	if (fsi < 0)
		fatal("%s: %s", special, strerror(errno));
	if (fstat(fsi, &st) < 0)
		fatal("%s: %s", special, strerror(errno));
	if (S_ISBLK(st.st_mode))
		fatal("%s: block device", special);
	if (!S_ISCHR(st.st_mode))
		warnx("%s: not a character-special device", special);
	cp = strchr(argv[0], '\0') - 1;
	if (cp == NULL ||
	    ((*cp < 'a' || *cp > ('a' + maxpartitions - 1))
	    && !isdigit((unsigned char)*cp)))
		fatal("%s: can't figure out file system partition", argv[0]);
	lp = getdisklabel(special, fsi);
	if (isdigit((unsigned char)*cp))
		pp = &lp->d_partitions[0];
	else
		pp = &lp->d_partitions[*cp - 'a'];
	if (DL_GETPSIZE(pp) == 0)
		fatal("%s: `%c' partition is unavailable", argv[0], *cp);
	if (pp->p_fstype == FS_BOOT)
		fatal("%s: `%c' partition overlaps boot program", argv[0], *cp);
havelabel:
	if (sectorsize == 0) {
		sectorsize = lp->d_secsize;
		if (sectorsize <= 0)
			fatal("%s: no default sector size", argv[0]);
	}

	if (fssize_input < 0) {
#if 1
		fatal("-s < 0 not yet implemented");
#else
		long long gap; /* leave gap at the end of partition */
		fssize_input = -fssize_input;
		if (fssize_usebytes) {
			gap = (daddr_t)fssize_input / (daddr_t)sectorsize;
			if ((daddr_t)fssize_input % (daddr_t)sectorsize != 0)
				gap++;
		} else
			gap = fssize_input;
		if (gap >= DL_GETPSIZE(pp))
			fatal("%s: requested gap of %lld sectors on partition "
			    "'%c' is too big", argv[0], gap, *cp);
		nsecs = DL_GETPSIZE(pp) - gap;
#endif
	} else {
		if (fssize_usebytes) {
			nsecs = fssize_input / sectorsize;
			if (fssize_input % sectorsize != 0)
				nsecs++;
		} else if (fssize_input == 0)
			nsecs = DL_GETPSIZE(pp);
		else
			nsecs = fssize_input;
	}

	if (nsecs > DL_GETPSIZE(pp))
	       fatal("%s: maximum file system size on the `%c' partition is "
		   "%llu sectors", argv[0], *cp, DL_GETPSIZE(pp));

	/* Can't use DL_SECTOBLK() because sectorsize may not be from label! */
	fssize = nsecs * (sectorsize / DEV_BSIZE);
	if (fsize == 0) {
		fsize = DISKLABELV1_FFS_FSIZE(pp->p_fragblock);
		if (fsize <= 0)
			fsize = MAX(DFL_FRAGSIZE, lp->d_secsize);
	}
	if (bsize == 0) {
		bsize = DISKLABELV1_FFS_BSIZE(pp->p_fragblock);
		if (bsize <= 0)
			bsize = MIN(DFL_BLKSIZE, 8 * fsize);
	}
	if (density == 0)
		density = NFPI * fsize;
	if (minfree < MINFREE && opt != FS_OPTSPACE && reqopt == -1) {
		warnx("warning: changing optimization to space "
		    "because minfree is less than %d%%\n", MINFREE);
		opt = FS_OPTSPACE;
	}
	if (maxbpg == 0) {
		if (Oflag <= 1)
			maxbpg = MAXBLKPG_FFS1(bsize);
		else
			maxbpg = MAXBLKPG_FFS2(bsize);
	}
	oldpartition = *pp;

	mkfs(pp, special, fsi, fso);
	if (!Nflag && memcmp(pp, &oldpartition, sizeof(oldpartition)))
		rewritelabel(special, fso, lp);
	if (!Nflag)
		close(fso);
	close(fsi);
	exit(0);
}

struct disklabel *
getdisklabel(char *s, int fd)
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
		if (disktype != NULL) {
			struct disklabel *lp;

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				fatal("%s: unknown disk type", disktype);
			return (lp);
		}
		warn("ioctl (GDINFO)");
		fatal("%s: can't read disk label; disk type must be specified",
		    s);
	}
	return (&lab);
}

void
rewritelabel(char *s, int fd, struct disklabel *lp)
{
	if (unlabeled)
		return;

	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		warn("ioctl (WDINFO)");
		fatal("%s: can't rewrite disk label", s);
	}
}

void
fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (fcntl(STDERR_FILENO, F_GETFL) < 0) {
		openlog(__progname, LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, fmt, ap);
		closelog();
	} else {
		vwarnx(fmt, ap);
	}
	va_end(ap);
	exit(1);
	/*NOTREACHED*/
}

__dead void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-Nq] [-b block-size] "
    "[-c fragments-per-cylinder-group] [-e maxbpg]\n"
    "\t[-f frag-size] [-g avgfilesize] [-h avgfpdir] [-i bytes]\n"
    "\t[-m free-space] [-O filesystem-format] [-o optimization]\n"
    "\t[-S sector-size] [-s size] [-T disktype] [-t fstype] "
    "special\n", __progname);

	exit(1);
}
