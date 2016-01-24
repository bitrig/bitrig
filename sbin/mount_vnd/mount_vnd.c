/*	$OpenBSD: mount_vnd.c,v 1.18 2016/01/24 01:02:24 gsoares Exp $	*/
/*
 * Copyright (c) 1993 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *
 * from: Utah $Hdr: vnconfig.c 1.1 93/12/15$
 *
 *	@(#)vnconfig.c	8.1 (Berkeley) 12/15/93
 */

#include <sys/param.h>	/* DEV_BSIZE */
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/disklabel.h>

#include <dev/biovar.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <util.h>

#define DEFAULT_VND	"vnd0"

#define VND_CONFIG	1
#define VND_UNCONFIG	2
#define VND_GET		3

int verbose = 0;
int run_mount_vnd = 0;

__dead void	 usage(void);
int		 config(char *, char *, int, struct disklabel *);
int		 getinfo(const char *);
int		 getinfo_all(void);

int
main(int argc, char **argv)
{
	int	 ch, rv, action, opt_c, opt_l, opt_u;
	char	*mntopts;
	extern char *__progname;
	struct disklabel *dp = NULL;

	if (strcasecmp(__progname, "mount_vnd") == 0)
		run_mount_vnd = 1;

	opt_c = opt_l = opt_u = 0;
	mntopts = NULL;
	action = VND_CONFIG;

	while ((ch = getopt(argc, argv, "clo:t:uv")) != -1) {
		switch (ch) {
		case 'c':
			opt_c = 1;
			break;
		case 'l':
			opt_l = 1;
			break;
		case 'o':
			mntopts = optarg;
			break;
		case 't':
			dp = getdiskbyname(optarg);
			if (dp == NULL)
				errx(1, "unknown disk type: %s", optarg);
			break;
		case 'u':
			opt_u = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (opt_c + opt_l + opt_u > 1)
		errx(1, "-c, -l and -u are mutually exclusive options");

	if (opt_l)
		action = VND_GET;
	else if (opt_u)
		action = VND_UNCONFIG;
	else
		action = VND_CONFIG;	/* default behavior */

	if (action == VND_CONFIG && argc == 2) {
		int ind_raw, ind_reg;

		/* fix order of arguments. */
		if (run_mount_vnd) {
			ind_raw = 1;
			ind_reg = 0;
		} else {
			ind_raw = 0;
			ind_reg = 1;
		}
		rv = config(argv[ind_raw], argv[ind_reg], action, dp);
	} else if (action == VND_UNCONFIG && argc == 1)
		rv = config(argv[0], NULL, action, NULL);
	else if (action == VND_GET && argc == 1) {
		rv = getinfo(argv[0]);
		if (rv == -1)
			err(4, "BIOCLOCATE");
	} else if (action == VND_GET && argc == 0)
		rv = getinfo_all();
	else
		usage();

	exit(rv);
}

char *
get_pkcs_key(char *arg, char *saltopt)
{
	char		 passphrase[128];
	char		 saltbuf[128], saltfilebuf[PATH_MAX];
	char		*key = NULL;
	char		*saltfile;
	const char	*errstr;
	int		 rounds;

	rounds = strtonum(arg, 1000, INT_MAX, &errstr);
	if (errstr)
		err(1, "rounds: %s", errstr);
	bzero(passphrase, sizeof(passphrase));
	if (readpassphrase("Encryption key: ", passphrase, sizeof(passphrase),
	    RPP_REQUIRE_TTY) == NULL)
		errx(1, "Unable to read passphrase");
	if (saltopt)
		saltfile = saltopt;
	else {
		printf("Salt file: ");
		fflush(stdout);
		saltfile = fgets(saltfilebuf, sizeof(saltfilebuf), stdin);
		if (saltfile)
			saltfile[strcspn(saltfile, "\n")] = '\0';
	}
	if (!saltfile || saltfile[0] == '\0') {
		warnx("Skipping salt file, insecure");
		memset(saltbuf, 0, sizeof(saltbuf));
	} else {
		int fd;

		fd = open(saltfile, O_RDONLY);
		if (fd == -1) {
			int *s;

			fprintf(stderr, "Salt file not found, attempting to "
			    "create one\n");
			fd = open(saltfile, O_RDWR|O_CREAT|O_EXCL, 0600);
			if (fd == -1)
				err(1, "Unable to create salt file: '%s'",
				    saltfile);
			for (s = (int *)saltbuf;
			    s < (int *)(saltbuf + sizeof(saltbuf)); s++)
				*s = arc4random();
			if (write(fd, saltbuf, sizeof(saltbuf))
			    != sizeof(saltbuf))
				err(1, "Unable to write salt file: '%s'",
				    saltfile);
			fprintf(stderr, "Salt file created as '%s'\n",
			    saltfile);
		} else {
			if (read(fd, saltbuf, sizeof(saltbuf))
			    != sizeof(saltbuf))
				err(1, "Unable to read salt file: '%s'",
				    saltfile);
		}
		close(fd);
	}
	if ((key = calloc(1, BLF_MAXUTILIZED)) == NULL)
		err(1, NULL);
	if (pkcs5_pbkdf2(passphrase, sizeof(passphrase), saltbuf,
	    sizeof (saltbuf), key, BLF_MAXUTILIZED, rounds))
		errx(1, "pkcs5_pbkdf2 failed");
	explicit_bzero(passphrase, 0, sizeof(passphrase));

	return (key);
}

int
getinfo(const char *vname)
{
	struct bio_locate bl;
	struct bioc_vndget vget;
	int fd;

	if ((fd = open("/dev/bio", O_RDWR)) < 0)
		err(1, "/dev/bio");

	bl.bl_name = (char *)vname;
	if (ioctl(fd, BIOCLOCATE, &bl) < 0) {
		if (errno == ENOENT)
			return (-1);
		err(4, "BIOCLOCATE");
	}

	vget.bvg_bio.bio_cookie = bl.bl_bio.bio_cookie;

	if (ioctl(fd, BIOCVNDGET, &vget) == -1)
		err(1, "ioctl: %s", vname);

	fprintf(stdout, "%s: ", vname);

	if (!vget.bvg_ino)
		fprintf(stdout, "not in use\n");
	else
		fprintf(stdout, "covering %s on %s, inode %llu\n",
		    vget.bvg_file, devname(vget.bvg_dev, S_IFBLK),
		    (unsigned long long)vget.bvg_ino);

	close(fd);
	return (0);
}

int
getinfo_all(void)
{
	char vname[7];
	unsigned char i;
	int rv;

	/*
	 * We don't know how many vnd devices are configured in the
	 * kernel, so iterate until we hit a non-existent.
	 */
	for (i = 0; i < UCHAR_MAX; i++) {
		(void)snprintf(vname, sizeof(vname), "vnd%d", i);
		rv = getinfo(vname);
		if (rv != 0) {
			if (rv == -1)	/* no such file */
				return (0);
			return (rv);
		}
	}

	return (0);
}

int
config(char *dev, char *file, int action, struct disklabel *dp)
{
	struct bio_locate bl;
	struct bioc_vndset vset;
	struct bioc_vndclr vclr;
	int fd, rv = -1;

	if ((fd = open("/dev/bio", O_RDWR)) < 0)
		err(4, "/dev/bio");

	bl.bl_name = dev;
	if (ioctl(fd, BIOCLOCATE, &bl) < 0)
		err(4, "BIOCLOCATE");

	/*
	 * Clear (un-configure) the device
	 */
	if (action == VND_UNCONFIG) {
		vclr.bvc_bio.bio_cookie = bl.bl_bio.bio_cookie;

		rv = ioctl(fd, BIOCVNDCLR, &vclr);
		if (rv)
			warn("BIOCVNDCLR");
		else if (verbose)
			printf("%s: cleared\n", dev);
	}
	/*
	 * Configure the device
	 */
	if (action == VND_CONFIG) {
		vset.bvs_bio.bio_cookie = bl.bl_bio.bio_cookie;
		vset.bvs_file = file;
		vset.bvs_secsize =
		    (dp && dp->d_secsize) ? dp->d_secsize : DEV_BSIZE;
		vset.bvs_nsectors =
		    (dp && dp->d_nsectors) ? dp->d_nsectors : 100;
		vset.bvs_ntracks = (dp && dp->d_ntracks) ? dp->d_ntracks : 1;

		rv = ioctl(fd, BIOCVNDSET, &vset);
		if (rv)
			warn("BIOCVNDSET");
		else if (verbose)
			printf("%s: %llu bytes on %s\n", dev, vset.bvs_size,
			    file);
	}

	close(fd);
	fflush(stdout);
 out:
	if (key)
		explicit_bzero(key, 0, keylen);
	return (rv < 0);
}

__dead void
usage(void)
{
	extern char *__progname;

	if (run_mount_vnd)
		(void)fprintf(stderr,
		    "usage: %s [-o options] [-t disktype] image vnd_dev\n",
		    __progname);
	else
		(void)fprintf(stderr,
		    "usage: %s [-cluv] [-t disktype] vnd_dev image\n",
		    __progname);

	exit(1);
}
