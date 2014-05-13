/*	$OpenBSD: dumpfs.c,v 1.30 2014/05/13 12:51:40 krw Exp $	*/

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
 * Copyright (c) 1983, 1992, 1993
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
#include <sys/time.h>

#include <sys/wapbl.h>
#include <sys/wapbl_replay.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ffs/fs.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

union {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define afs	fsun.fs
 
union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define acg	cgun.cg

union {
	struct wapbl_wc_header wh;
	struct wapbl_wc_null wn;
	char pad[MAXBSIZE];
} jbuf;
#define awh	jbuf.wh
#define awn	jbuf.wn

#define offsetof(s, e)	((size_t)&((s *)0)->e)

int			dumpfs(int, const char *);
int			dumpcg(const char *, int, int);
int			marshal(const char *);
int			open_disk(const char *);
void			pbits(void *, int);
int			print_journal(const char *, int);
const char *		wapbl_type_string(unsigned);
void			print_journal_header(const char *);
off_t			print_journal_entries(const char *, size_t);
__dead static void	usage(void);

int dojournal = 0;

int
main(int argc, char *argv[])
{
	struct fstab *fs;
	const char *name;
	int ch, domarshal, eval, fd;

	domarshal = eval = 0;

	while ((ch = getopt(argc, argv, "jm")) != -1) {
		switch (ch) {
		case 'j':	/* journal */
			dojournal = 1;
			break;
		case 'm':
			domarshal = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	for (; *argv != NULL; argv++) {
		if ((fs = getfsfile(*argv)) != NULL)
			name = fs->fs_spec;
		else
			name = *argv;
		if ((fd = open_disk(name)) == -1) {
			eval |= 1;
			continue;
		}
		if (domarshal)
			eval |= marshal(name);
		else
			eval |= dumpfs(fd, name);
		close(fd);
	}
	exit(eval);
}

int
open_disk(const char *name)
{
	int fd, i, sbtry[] = SBLOCKSEARCH;
	ssize_t n;

	/* XXX - should retry w/raw device on failure */
	if ((fd = opendev(name, O_RDONLY, 0, NULL)) < 0) {
		warn("%s", name);
		return(-1);
	}

	/* Read superblock, could be UFS1 or UFS2. */
	for (i = 0; sbtry[i] != -1; i++) {
		n = pread(fd, &afs, SBLOCKSIZE, (off_t)sbtry[i]);
		if (n == SBLOCKSIZE && (afs.fs_magic == FS_UFS1_MAGIC ||
		    (afs.fs_magic == FS_UFS2_MAGIC &&
		    afs.fs_sblockloc == sbtry[i])) &&
		    afs.fs_bsize <= MAXBSIZE &&
		    afs.fs_bsize >= sizeof(struct fs))
			break;
	}
	if (sbtry[i] == -1) {
		warnx("cannot find filesystem superblock");
		close(fd);
		return (-1);
	}

	return (fd);
}

int
dumpfs(int fd, const char *name)
{
	time_t fstime;
	int64_t fssize;
	int32_t fsflags;
	size_t size;
	off_t off;
	int i, j;

	switch (afs.fs_magic) {
	case FS_UFS2_MAGIC:
		fssize = afs.fs_size;
		fstime = afs.fs_time;
		printf("magic\t%x (FFS2)\ttime\t%s",
		    afs.fs_magic, ctime(&fstime));
		printf("superblock location\t%jd\tid\t[ %x %x ]\n",
		    (intmax_t)afs.fs_sblockloc, afs.fs_id[0], afs.fs_id[1]);
		printf("ncg\t%d\tsize\t%jd\tblocks\t%jd\n",
		    afs.fs_ncg, (intmax_t)fssize, (intmax_t)afs.fs_dsize);
		break;
	case FS_UFS1_MAGIC:
		fssize = afs.fs_ffs1_size;
		fstime = afs.fs_ffs1_time;
		printf("magic\t%x (FFS1)\ttime\t%s",
		    afs.fs_magic, ctime(&fstime));
		printf("id\t[ %x %x ]\n", afs.fs_id[0], afs.fs_id[1]);
		i = 0;
		if (afs.fs_postblformat != FS_42POSTBLFMT) {
			i++;
			if (afs.fs_inodefmt >= FS_44INODEFMT) {
				size_t max;

				i++;
				max = afs.fs_maxcontig;
				size = afs.fs_contigsumsize;
				if ((max < 2 && size == 0) ||
				    (max > 1 && size >= MIN(max, FS_MAXCONTIG)))
					i++;
			}
		}
		printf("cylgrp\t%s\tinodes\t%s\tfslevel %d\n",
		    i < 1 ? "static" : "dynamic",
		    i < 2 ? "4.2/4.3BSD" : "4.4BSD", i);
		printf("ncg\t%d\tncyl\t%d\tsize\t%d\tblocks\t%d\n",
		    afs.fs_ncg, afs.fs_ncyl, afs.fs_ffs1_size, afs.fs_ffs1_dsize);
		break;
	default:
		goto err;
	}
	printf("bsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    afs.fs_bsize, afs.fs_bshift, afs.fs_bmask);
	printf("fsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    afs.fs_fsize, afs.fs_fshift, afs.fs_fmask);
	printf("frag\t%d\tshift\t%d\tfsbtodb\t%d\n",
	    afs.fs_frag, afs.fs_fragshift, afs.fs_fsbtodb);
	printf("minfree\t%d%%\toptim\t%s\tsymlinklen %d\n",
	    afs.fs_minfree, afs.fs_optim == FS_OPTSPACE ? "space" : "time",
	    afs.fs_maxsymlinklen);
	switch (afs.fs_magic) {
	case FS_UFS2_MAGIC:
		printf("%s %d\tmaxbpg\t%d\tmaxcontig %d\tcontigsumsize %d\n",
		    "maxbsize", afs.fs_maxbsize, afs.fs_maxbpg,
		    afs.fs_maxcontig, afs.fs_contigsumsize);
		printf("nbfree\t%jd\tndir\t%jd\tnifree\t%jd\tnffree\t%jd\n",
		    (intmax_t)afs.fs_cstotal.cs_nbfree, 
		    (intmax_t)afs.fs_cstotal.cs_ndir,
		    (intmax_t)afs.fs_cstotal.cs_nifree, 
		    (intmax_t)afs.fs_cstotal.cs_nffree);
		printf("bpg\t%d\tfpg\t%d\tipg\t%d\n",
		    afs.fs_fpg / afs.fs_frag, afs.fs_fpg, afs.fs_ipg);
		printf("nindir\t%d\tinopb\t%d\tmaxfilesize\t%ju\n",
		    afs.fs_nindir, afs.fs_inopb, 
		    (uintmax_t)afs.fs_maxfilesize);
		printf("sbsize\t%d\tcgsize\t%d\tcsaddr\t%jd\tcssize\t%d\n",
		    afs.fs_sbsize, afs.fs_cgsize, (intmax_t)afs.fs_csaddr,
		    afs.fs_cssize);
		break;
	case FS_UFS1_MAGIC:
		printf("maxbpg\t%d\tmaxcontig %d\tcontigsumsize %d\n",
		    afs.fs_maxbpg, afs.fs_maxcontig, afs.fs_contigsumsize);
		printf("nbfree\t%d\tndir\t%d\tnifree\t%d\tnffree\t%d\n",
		    afs.fs_ffs1_cstotal.cs_nbfree, afs.fs_ffs1_cstotal.cs_ndir,
		    afs.fs_ffs1_cstotal.cs_nifree, afs.fs_ffs1_cstotal.cs_nffree);
		printf("cpg\t%d\tbpg\t%d\tfpg\t%d\tipg\t%d\n",
		    afs.fs_cpg, afs.fs_fpg / afs.fs_frag, afs.fs_fpg,
		    afs.fs_ipg);
		printf("nindir\t%d\tinopb\t%d\tnspf\t%d\tmaxfilesize\t%ju\n",
		    afs.fs_nindir, afs.fs_inopb, afs.fs_nspf,
		    (uintmax_t)afs.fs_maxfilesize);
		printf("sbsize\t%d\tcgsize\t%d\tcgoffset %d\tcgmask\t0x%08x\n",
		    afs.fs_sbsize, afs.fs_cgsize, afs.fs_cgoffset,
		    afs.fs_cgmask);
		printf("csaddr\t%d\tcssize\t%d\n",
		    afs.fs_ffs1_csaddr, afs.fs_cssize);
		printf("rotdelay %dms\trps\t%d\tinterleave %d\n",
		    afs.fs_rotdelay, afs.fs_rps, afs.fs_interleave);
		printf("nsect\t%d\tnpsect\t%d\tspc\t%d\n",
		    afs.fs_nsect, afs.fs_npsect, afs.fs_spc);
		break;
	default:
		goto err;
	}
	printf("sblkno\t%d\tcblkno\t%d\tiblkno\t%d\tdblkno\t%d\n",
	    afs.fs_sblkno, afs.fs_cblkno, afs.fs_iblkno, afs.fs_dblkno);
	printf("cgrotor\t%d\tfmod\t%d\tronly\t%d\tclean\t%d\n",
	    afs.fs_cgrotor, afs.fs_fmod, afs.fs_ronly, afs.fs_clean);
	printf("avgfpdir %d\tavgfilesize %d\n",
	    afs.fs_avgfpdir, afs.fs_avgfilesize);
	if (dojournal) {
		printf("wapbl version 0x%x\tlocation %u\tflags 0x%x\n",
		    afs.fs_journal_version, afs.fs_journal_location,
		    afs.fs_journal_flags);
		printf("wapbl loc0 %llu\tloc1 %llu",
		    afs.fs_journallocs[0], afs.fs_journallocs[1]);
		printf("\tloc2 %llu\tloc3 %llu\n",
		    afs.fs_journallocs[2], afs.fs_journallocs[3]);
	}
	printf("flags\t");
	if (afs.fs_magic == FS_UFS2_MAGIC ||
	    afs.fs_ffs1_flags & FS_FLAGS_UPDATED)
		fsflags = afs.fs_flags;
	else
		fsflags = afs.fs_ffs1_flags;
	if (fsflags == 0)
		printf("none");
	if (fsflags & FS_UNCLEAN)
		printf("unclean ");
	if (fsflags & FS_DOSOFTDEP)
		printf("soft-updates ");
	if (fsflags & FS_FLAGS_UPDATED)
		printf("updated ");
	if (fsflags & FS_DOWAPBL)
		printf("wapbl ");
#if 0
	fsflags &= ~(FS_UNCLEAN | FS_DOSOFTDEP | FS_FLAGS_UPDATED);
	if (fsflags != 0)
		printf("unknown flags (%#x)", fsflags);
#endif
	putchar('\n');
	printf("fsmnt\t%s\n", afs.fs_fsmnt);
	printf("volname\t%s\tswuid\t%ju\n",
		afs.fs_volname, (uintmax_t)afs.fs_swuid);
	if (dojournal) {
		printf("\n");
		print_journal(name, fd);
	}
	printf("\ncs[].cs_(nbfree,ndir,nifree,nffree):\n\t");
	afs.fs_csp = calloc(1, afs.fs_cssize);
	for (i = 0, j = 0; i < afs.fs_cssize; i += afs.fs_bsize, j++) {
		size = afs.fs_cssize - i < afs.fs_bsize ?
		    afs.fs_cssize - i : afs.fs_bsize;
		off = (off_t)(fsbtodb(&afs, (afs.fs_csaddr + j *
		    afs.fs_frag))) * DEV_BSIZE;
		if (pread(fd, (char *)afs.fs_csp + i, size, off) != size)
			goto err;
	}
	for (i = 0; i < afs.fs_ncg; i++) {
		struct csum *cs = &afs.fs_cs(&afs, i);
		if (i && i % 4 == 0)
			printf("\n\t");
		printf("(%d,%d,%d,%d) ",
		    cs->cs_nbfree, cs->cs_ndir, cs->cs_nifree, cs->cs_nffree);
	}
	printf("\n");
	if (fssize % afs.fs_fpg) {
		if (afs.fs_magic == FS_UFS1_MAGIC)
			printf("cylinders in last group %d\n",
			    howmany(afs.fs_ffs1_size % afs.fs_fpg,
			    afs.fs_spc / afs.fs_nspf));
		printf("blocks in last group %ld\n\n",
		    (long)((fssize % afs.fs_fpg) / afs.fs_frag));
	}
	for (i = 0; i < afs.fs_ncg; i++)
		if (dumpcg(name, fd, i))
			goto err;
	return (0);

err:	warn("%s", name);
	return (1);
}

int
dumpcg(const char *name, int fd, int c)
{
	time_t cgtime;
	off_t cur;
	int i, j;

	printf("\ncg %d:\n", c);
	cur = (off_t)fsbtodb(&afs, cgtod(&afs, c)) * DEV_BSIZE;
	if (pread(fd, &acg, afs.fs_bsize, cur) != afs.fs_bsize) {
		warn("%s: error reading cg", name);
		return(1);
	}
	switch (afs.fs_magic) {
	case FS_UFS2_MAGIC:
		cgtime = acg.cg_ffs2_time;
		printf("magic\t%x\ttell\t%jx\ttime\t%s",
		    acg.cg_magic, (intmax_t)cur, ctime(&cgtime));
		printf("cgx\t%d\tndblk\t%d\tniblk\t%d\tinitiblk %d\n",
		    acg.cg_cgx, acg.cg_ndblk, acg.cg_ffs2_niblk,
		    acg.cg_initediblk);
		break;
	case FS_UFS1_MAGIC:
		cgtime = acg.cg_time;
		printf("magic\t%x\ttell\t%jx\ttime\t%s",
		    afs.fs_postblformat == FS_42POSTBLFMT ?
		    ((struct ocg *)&acg)->cg_magic : acg.cg_magic,
		    (intmax_t)cur, ctime(&cgtime));
		printf("cgx\t%d\tncyl\t%d\tniblk\t%d\tndblk\t%d\n",
		    acg.cg_cgx, acg.cg_ncyl, acg.cg_niblk, acg.cg_ndblk);
		break;
	default:
		break;
	}
	printf("nbfree\t%d\tndir\t%d\tnifree\t%d\tnffree\t%d\n",
	    acg.cg_cs.cs_nbfree, acg.cg_cs.cs_ndir,
	    acg.cg_cs.cs_nifree, acg.cg_cs.cs_nffree);
	printf("rotor\t%d\tirotor\t%d\tfrotor\t%d\nfrsum",
	    acg.cg_rotor, acg.cg_irotor, acg.cg_frotor);
	for (i = 1, j = 0; i < afs.fs_frag; i++) {
		printf("\t%d", acg.cg_frsum[i]);
		j += i * acg.cg_frsum[i];
	}
	printf("\nsum of frsum: %d", j);
	if (afs.fs_contigsumsize > 0) {
		for (i = 1; i < afs.fs_contigsumsize; i++) {
			if ((i - 1) % 8 == 0)
				printf("\nclusters %d-%d:", i,
				    afs.fs_contigsumsize - 1 < i + 7 ?
				    afs.fs_contigsumsize - 1 : i + 7);
			printf("\t%d", cg_clustersum(&acg)[i]);
		}
		printf("\nclusters size %d and over: %d\n",
		    afs.fs_contigsumsize,
		    cg_clustersum(&acg)[afs.fs_contigsumsize]);
		printf("clusters free:\t");
		pbits(cg_clustersfree(&acg), acg.cg_nclusterblks);
	} else
		printf("\n");
	printf("inodes used:\t");
	pbits(cg_inosused(&acg), afs.fs_ipg);
	printf("blks free:\t");
	pbits(cg_blksfree(&acg), afs.fs_fpg);
#if 0
	/* XXX - keep this? */
	if (afs.fs_magic == FS_UFS1_MAGIC) {
		printf("b:\n");
		for (i = 0; i < afs.fs_cpg; i++) {
			if (cg_blktot(&acg)[i] == 0)
				continue;
			printf("   c%d:\t(%d)\t", i, cg_blktot(&acg)[i]);
			printf("\n");
		}
	}
#endif
	return (0);
}

int
marshal(const char *name)
{
	int Oflag;

	printf("# newfs command for %s\n", name);
	printf("newfs ");
	if (afs.fs_volname[0] != '\0')
		printf("-L %s ", afs.fs_volname);

	Oflag = (afs.fs_magic == FS_UFS2_MAGIC) +
	    (afs.fs_inodefmt == FS_44INODEFMT);
	printf("-O %d ", Oflag);
	printf("-b %d ", afs.fs_bsize);
	/* -c unimplemented */
	printf("-e %d ", afs.fs_maxbpg);
	printf("-f %d ", afs.fs_fsize);
	printf("-g %d ", afs.fs_avgfilesize);
	printf("-h %d ", afs.fs_avgfpdir);
	/* -i unimplemented */
	printf("-m %d ", afs.fs_minfree);
	printf("-o ");
	switch (afs.fs_optim) {
	case FS_OPTSPACE:
		printf("space ");
		break;
	case FS_OPTTIME:
		printf("time ");
		break;
	default:
		printf("unknown ");
		break;
	}
	/* -S unimplemented */
	printf("-s %jd ", (intmax_t)afs.fs_size * (afs.fs_fsize / DEV_BSIZE));
	printf("%s ", name);
	printf("\n");

	return 0;
}

void
pbits(void *vp, int max)
{
	int i;
	char *p;
	int count, j;

	for (count = i = 0, p = vp; i < max; i++)
		if (isset(p, i)) {
			if (count)
				printf(",%s", count % 6 ? " " : "\n\t");
			count++;
			printf("%d", i);
			j = i;
			while ((i+1)<max && isset(p, i+1))
				i++;
			if (i != j)
				printf("-%d", i);
		}
	printf("\n");
}

int
print_journal(const char *name, int fd)
{
	daddr_t off;
	size_t count, blklen, bno, skip;
	off_t boff, head, tail, len;
	uint32_t generation;

	if (afs.fs_journal_version != UFS_WAPBL_VERSION)
		return 0;

	generation = 0;
	head = tail = 0;

	switch (afs.fs_journal_location) {
	case UFS_WAPBL_JOURNALLOC_END_PARTITION:
	case UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM:

		off    = afs.fs_journallocs[0];
		count  = afs.fs_journallocs[1];
		blklen = afs.fs_journallocs[2];

		for (bno=0; bno<count; bno += skip / blklen) {

			skip = blklen;

			boff = bno * blklen;
			if (bno * blklen >= 2 * blklen &&
			  ((head >= tail && (boff < tail || boff >= head)) ||
			  (head < tail && (boff >= head && boff < tail))))
				continue;

			printf("journal block %lu offset %lld\n",
				(unsigned long)bno, (long long) boff);

			if (lseek(fd, (off_t)(off*blklen) + boff, SEEK_SET)
			    == (off_t)-1)
				return (1);
			if (read(fd, &jbuf, blklen) != (ssize_t)blklen) {
				warnx("%s: error reading journal", name);
				return 1;
			}

			switch (awh.wc_type) {
			case 0:
				break;
			case WAPBL_WC_HEADER:
				print_journal_header(name);
				if (awh.wc_generation > generation) {
					head = awh.wc_head;
					tail = awh.wc_tail;
				}
				generation = awh.wc_generation;
				skip = awh.wc_len;
				break;
			default:
				len = print_journal_entries(name, blklen);
				skip = awh.wc_len;
				if (len != (off_t)skip)
					printf("  CORRUPTED RECORD\n");
				break;
			}

			if (blklen == 0)
				break;

			skip = (skip + blklen - 1) / blklen * blklen;
			if (skip == 0)
				break;

		}
		break;
	}

	return 0;
}

const char *
wapbl_type_string(unsigned t)
{
	static char buf[12];

	switch (t) {
	case WAPBL_WC_BLOCKS:
		return "blocks";
	case WAPBL_WC_REVOCATIONS:
		return "revocations";
	case WAPBL_WC_INODES:
		return "inodes";
	case WAPBL_WC_HEADER:
		return "header";
	}

	snprintf(buf,sizeof(buf),"%08x",t);
	return buf;
}

void
print_journal_header(const char *name)
{
	printf("  type %s len %d  version %u\n",
		wapbl_type_string(awh.wc_type), awh.wc_len,
		awh.wc_version);
	printf("  checksum      %08x  generation %9u\n",
		awh.wc_checksum, awh.wc_generation);
	printf("  fsid %08x.%08x  time %llu nsec %u\n",
		awh.wc_fsid[0], awh.wc_fsid[1],
		(unsigned long long)awh.wc_time, awh.wc_timensec);
	printf("  log_bshift  %10u  fs_bshift %10u\n",
		awh.wc_log_dev_bshift, awh.wc_fs_dev_bshift);
	printf("  head        %10lld  tail      %10lld\n",
		(long long)awh.wc_head, (long long)awh.wc_tail);
	printf("  circ_off    %10lld  circ_size %10lld\n",
		(long long)awh.wc_circ_off, (long long)awh.wc_circ_size);
}

off_t
print_journal_entries(const char *name, size_t blklen)
{
	int i, n;
	struct wapbl_wc_blocklist *wcb;
	struct wapbl_wc_inodelist *wci;
	off_t len = 0;
	int ph;

	printf("  type %s len %d",
		wapbl_type_string(awn.wc_type), awn.wc_len);

	switch (awn.wc_type) {
	case WAPBL_WC_BLOCKS:
	case WAPBL_WC_REVOCATIONS:
		wcb = (struct wapbl_wc_blocklist *)&awn;
		printf("  blkcount %u\n", wcb->wc_blkcount);
		ph = (blklen - offsetof(struct wapbl_wc_blocklist, wc_blocks))
			/ sizeof(wcb->wc_blocks[0]);
		n = MIN(wcb->wc_blkcount, ph);
		for (i=0; i<n; i++) {
			if (/* verbose */ 1) {
				printf("  %3d: daddr %14llu  dlen %d\n", i,
				 (unsigned long long)wcb->wc_blocks[i].wc_daddr,
				 wcb->wc_blocks[i].wc_dlen);
			}
			len += wcb->wc_blocks[i].wc_dlen;
		}
		if (awn.wc_type == WAPBL_WC_BLOCKS) {
			if (len % blklen)
				len += blklen - len % blklen;
		} else
			len = 0;
		break;
	case WAPBL_WC_INODES:
		wci = (struct wapbl_wc_inodelist *)&awn;
		printf("  count %u clear %u\n",
			wci->wc_inocnt, wci->wc_clear);
		ph = (blklen - offsetof(struct wapbl_wc_inodelist, wc_inodes))
			/ sizeof(wci->wc_inodes[0]);
		n = MIN(wci->wc_inocnt, ph);
		for (i=0; i<n; ++i) {
			if (/* verbose */ 1) {
				printf("  %3d: inumber %10u  imode %08x\n", i,
					wci->wc_inodes[i].wc_inumber,
					wci->wc_inodes[i].wc_imode);
			}
		}
		break;
	default:
		printf("\n");
		break;
	}

	return len + blklen;
}

__dead void
usage(void)
{
	(void)fprintf(stderr, "usage: dumpfs [-jm] filesys | device\n");
	exit(1);
}
