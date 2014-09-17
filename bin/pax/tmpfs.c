/*
 * Copyright (c) 2014 Martin Natano <natano@natano.net>
 * Copyright (c) 2014 Pedro Martelletto <pedro@ambientworks.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/tree.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <tmpfs/tmpfs_snapshot.h>

#include "pax.h"
#include "extern.h"

#define ROOT_ID	2

struct icachenode {
	SPLAY_ENTRY(icachenode)	entry;
	char			path[PAXPATHLEN + 1];
	uint64_t		id;
	struct stat		sb;
};

struct dcachenode {
	SPLAY_ENTRY(dcachenode)	entry;
	char			path[PAXPATHLEN + 1];
	uint64_t		id;
};

struct idmapnode {
	SPLAY_ENTRY(idmapnode)	entry;
	ino_t			from;
	uint64_t		to;
};

static SPLAY_HEAD(icache, icachenode)	icache = SPLAY_INITIALIZER(&icache);
static SPLAY_HEAD(dcache, dcachenode)	dcache = SPLAY_INITIALIZER(&dcache);
static SPLAY_HEAD(idmap, idmapnode)	idmap = SPLAY_INITIALIZER(&idmap);
static off_t				bytes_written;
static uint64_t				highest_inode;

static int	idcmp(struct icachenode *, struct icachenode *);
static int	pathcmp(struct dcachenode *, struct dcachenode *);
static int	fromcmp(struct idmapnode *, struct idmapnode *);

SPLAY_PROTOTYPE_STATIC(icache, icachenode, entry, idcmp);
SPLAY_PROTOTYPE_STATIC(dcache, dcachenode, entry, pathcmp);
SPLAY_PROTOTYPE_STATIC(idmap, idmapnode, entry, fromcmp);

static struct icachenode	*icache_find(uint64_t);
static int			 icache_insert(uint64_t, const char *,
				    struct stat *);
static const char		*normpath(const char *);
static uint64_t			 dcache_parent_id(const char *);
static int			 dcache_insert(const char *, uint64_t);
static uint64_t			 idmap_translate(ino_t);
static int			 hdr_set_type(tmpfs_snap_node_t *, mode_t);

static int
idcmp(struct icachenode *in1, struct icachenode *in2)
{
	if (in1->id < in2->id)
		return (-1);

	return (in1->id != in2->id);
}

static int
pathcmp(struct dcachenode *dn1, struct dcachenode *dn2)
{
	return strcmp(dn1->path, dn2->path);
}

static int
fromcmp(struct idmapnode *idn1, struct idmapnode *idn2)
{
	if (idn1->from < idn2->from)
		return (-1);

	return (idn1->from != idn2->from);
}

SPLAY_GENERATE_STATIC(icache, icachenode, entry, idcmp);
SPLAY_GENERATE_STATIC(dcache, dcachenode, entry, pathcmp);
SPLAY_GENERATE_STATIC(idmap, idmapnode, entry, fromcmp);


/*
 * icache: Remember inode number and path of files read from an archive. Used
 * to find the path of parent directories and for hardlink handling.
 */

static struct icachenode *
icache_find(uint64_t ino)
{
	struct icachenode find;

	find.id = ino;

	return (SPLAY_FIND(icache, &icache, &find));
}

static int
icache_insert(uint64_t ino, const char *path, struct stat *sb)
{
	struct icachenode *node;

	node = calloc(1, sizeof(*node));
	if (node == NULL) {
		syswarn(1, errno, "calloc");
		return (-1);
	}

	node->id = ino;
	strlcpy(node->path, path, sizeof(node->path));
	memcpy(&node->sb, sb, sizeof(node->sb));
	SPLAY_INSERT(icache, &icache, node);

	return (0);
}


/*
 * dcache: Remember inode number and path of archived file. Used to find the
 * parent inode number for a given path.
 */

static const char *
normpath(const char *path)
{
	if (path[0] == '/')
		path++;
	else if (strncmp(path, "./", 2) == 0)
		path += 2;

	return (path);
}

static uint64_t
dcache_parent_id(const char *path)
{
	struct dcachenode find, *node;
	char *slash;

	strlcpy(find.path, normpath(path), sizeof(find.path));
	slash = strrchr(find.path, '/');
	if (slash == NULL)
		return (ROOT_ID);
	*slash = '\0';

	node = SPLAY_FIND(dcache, &dcache, &find);
	if (node == NULL) {
		paxwarn(1, "%s doesn't have a parent; attaching to root node",
		    path);
		return (ROOT_ID);
	}

	return (node->id);
}

static int
dcache_insert(const char *path, uint64_t id)
{
	struct dcachenode *node;

	node = calloc(1, sizeof(*node));
	if (node == NULL) {
		syswarn(1, errno, "calloc");
		return (-1);
	}

	strlcpy(node->path, normpath(path), sizeof(node->path));
	node->id = id;
	SPLAY_INSERT(dcache, &dcache, node);

	return (0);
}


/*
 * idmap: Translate file system inode numbers to sequential numbers for the
 * snapshot. Valid numbers in the snapshot must be greater than 2. Return 0 on
 * error.
 */

static uint64_t
idmap_translate(ino_t from)
{
	struct idmapnode find, *node;

	find.from = from;
	node = SPLAY_FIND(idmap, &idmap, &find);
	if (node == NULL) {
		node = calloc(1, sizeof(*node));
		if (node == NULL) {
			syswarn(1, errno, "calloc");
			return (0);
		}

		node->from = from;
		node->to = ++highest_inode;
		if (node->to == 0) {
			paxwarn(1,
			    "couldn't allocate inode number; too many files");
			free(node);
			return (0);
		}

		SPLAY_INSERT(idmap, &idmap, node);
	}

	return (node->to);
}

static int
hdr_set_type(tmpfs_snap_node_t *hdr, mode_t mode)
{
	if (S_ISBLK(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_BLK;
	else if (S_ISCHR(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_CHR;
	else if (S_ISDIR(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_DIR;
	else if (S_ISFIFO(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_FIFO;
	else if (S_ISLNK(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_LNK;
	else if (S_ISREG(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_REG;
	else if (S_ISSOCK(mode))
		hdr->tsn_type = TMPFS_SNAP_NTYPE_SOCK;
	else {
		paxwarn(1, "unknown file mode=%u", mode);
		return (-1);
	}

	return (0);
}

int
tmpfs_id(char *blk, int size)
{
	tmpfs_snap_t *hdr;

	if (size < sizeof(*hdr))
		return (-1);

	hdr = (tmpfs_snap_t *)blk;
	TMPFS_SNAP_NTOH(hdr);

	if (strcmp(hdr->ts_magic, TMPFS_SNAP_MAGIC) ||
	    hdr->ts_version != TMPFS_SNAP_VERSION)
		return (-1);

	force_one_volume = 1;

	return (0);
}

int
tmpfs_strd(void)
{
	return (rd_skip(sizeof(tmpfs_snap_t)));
}

/*
 * This passes us a pointer (buf -- why not const?) to a tmpfs snapshot entry
 * in memory to be parsed into an ARCHD pointer, allowing us to unpack a tmpfs
 * snapshot. The size of buf is as specified by 'hsz' in the tmpfs FSUB.
 */
int
tmpfs_rd(ARCHD *arcn, char *buf)
{
	tmpfs_snap_node_t *hdr;
	struct icachenode *parent, *node;
	int nlen;

	hdr = (tmpfs_snap_node_t *)buf;
	TMPFS_SNAP_NODE_NTOH(hdr);

	memset(arcn, 0, sizeof(*arcn));
	arcn->org_name = arcn->name;
	arcn->sb.st_nlink = 1;

	if (hdr->tsn_id < ROOT_ID) {
		paxwarn(1, "invalid tsn_id=%llu", hdr->tsn_id);
		return (-1);
	}

	if (hdr->tsn_id == ROOT_ID) {
		arcn->name[0] = '.';
		arcn->name[1] = '\0';
		arcn->nlen = 1;
	} else {
		parent = icache_find(hdr->tsn_parent);
		if (parent == NULL) {
			paxwarn(1, "could not find parent %llu of %llu",
			    hdr->tsn_parent, hdr->tsn_id);
			return (-1);
		}

		nlen = snprintf(arcn->name, sizeof(arcn->name),
		    "%s/%s", parent->path, hdr->tsn_name);
		if (nlen < 0 || (size_t)nlen >= sizeof(arcn->name)) {
			paxwarn(1, "path too long for %llu", hdr->tsn_id);
			return (-1);
		}
		arcn->nlen = nlen;
	}

	node = icache_find(hdr->tsn_id);
	if (node != NULL) {
		/* file was seen before, which means it is a hardlink */
		memcpy(&arcn->sb, &node->sb, sizeof(arcn->sb));
		arcn->type = (arcn->sb.st_mode & S_IFREG) ? PAX_HRG : PAX_HLK;
		arcn->ln_nlen = (int)strlen(node->path);
		memcpy(arcn->ln_name, node->path, arcn->ln_nlen);
		return (0);
	}

	arcn->sb.st_mode = hdr->tsn_mode;
	arcn->sb.st_uid = hdr->tsn_uid;
	arcn->sb.st_gid = hdr->tsn_gid;

	/* XXX: pax ignores file flags */
	arcn->sb.st_flags = tmpfs_snap_flags_node(hdr->tsn_flags);

	arcn->sb.st_atim = TMPFS_TST_TO_TS(hdr->tsn_atime);
	arcn->sb.st_mtim = TMPFS_TST_TO_TS(hdr->tsn_mtime);
	arcn->sb.st_ctim = TMPFS_TST_TO_TS(hdr->tsn_ctime);

	switch(hdr->tsn_type) {
	case TMPFS_SNAP_NTYPE_REG:
		arcn->type = PAX_REG;
		arcn->sb.st_mode |= S_IFREG;
		arcn->skip = arcn->sb.st_size = hdr->tsn_spec.tsn_size;
		break;
	case TMPFS_SNAP_NTYPE_DIR:
		arcn->type = PAX_DIR;
		arcn->sb.st_mode |= S_IFDIR;
		arcn->sb.st_nlink = 2;
		break;
	case TMPFS_SNAP_NTYPE_BLK:
		arcn->type = PAX_BLK;
		arcn->sb.st_mode |= S_IFBLK;
		arcn->sb.st_rdev = hdr->tsn_spec.tsn_dev;
		break;
	case TMPFS_SNAP_NTYPE_CHR:
		arcn->type = PAX_CHR;
		arcn->sb.st_mode |= S_IFCHR;
		arcn->sb.st_rdev = hdr->tsn_spec.tsn_dev;
		break;
	case TMPFS_SNAP_NTYPE_LNK:
		arcn->type = PAX_SLK;
		arcn->sb.st_mode |= S_IFLNK;
		arcn->sb.st_size = hdr->tsn_spec.tsn_size;
		if (arcn->sb.st_size >= sizeof(arcn->ln_name)) {
			paxwarn(1, "symlink too long (%llu bytes)",
			    hdr->tsn_spec.tsn_size);
			return (-1);
		}
		arcn->ln_nlen = (int)arcn->sb.st_size;
		if (rd_wrbuf(arcn->ln_name, arcn->ln_nlen) != arcn->ln_nlen) {
			paxwarn(1, "rd_wrbuf() failed");
			return (-1);
		}
		break;
	case TMPFS_SNAP_NTYPE_SOCK:
		arcn->type = PAX_SCK;
		arcn->sb.st_mode |= S_IFSOCK;
		break;
	case TMPFS_SNAP_NTYPE_FIFO:
		arcn->type = PAX_FIF;
		arcn->sb.st_mode |= S_IFIFO;
		break;
	default:
		paxwarn(1, "unknown tsn_type=%d", hdr->tsn_type);
		return (-1);
	}

	return (icache_insert(hdr->tsn_id, arcn->name, &arcn->sb));
}

off_t
tmpfs_endrd(void)
{
	struct icachenode *in;

	while ((in = SPLAY_ROOT(&icache)) != NULL) {
		SPLAY_REMOVE(icache, &icache, in);
		free(in);
	}

	return (0);
}

int
tmpfs_stwr(void)
{
	tmpfs_snap_node_t hdr;
	struct timespec ts;

	/* Skip header; it will be written in tmpfs_endwr(). */
	wr_skip(sizeof(tmpfs_snap_t));
	bytes_written = sizeof(tmpfs_snap_t);

	highest_inode = ROOT_ID;

	/*
	 * The tmpfs snapshot format always starts with the root directory
	 * node. pax can be invoked with either one of more directories or
	 * files as input (or a mix of both), so we have to make up a fake
	 * root directory node, in order to conform to the format.
	 */
	memset(&hdr, 0, sizeof(hdr));
	strlcpy(hdr.tsn_name, "/", sizeof(hdr.tsn_name));
	hdr.tsn_id = ROOT_ID;
	hdr.tsn_parent = ROOT_ID;
	hdr.tsn_uid = 0;
	hdr.tsn_gid = 0;
	hdr.tsn_mode = S_IFDIR | 0755;
	hdr.tsn_type = TMPFS_SNAP_NTYPE_DIR;

	/*
	 * tsn_ctime is used by ramdisk kernels as a fallback value for
	 * initializing the system clock.
	 */
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		syswarn(1, errno, "failed to get time from realtime clock");
		return (-1);
	}

	hdr.tsn_atime = TMPFS_TS_TO_TST(ts);
	hdr.tsn_mtime = TMPFS_TS_TO_TST(ts);
	hdr.tsn_ctime = TMPFS_TS_TO_TST(ts);

	TMPFS_SNAP_NODE_HTON(&hdr);
	if (wr_rdbuf((char *)&hdr, sizeof(hdr)) == -1)
		return (-1);

	bytes_written += sizeof(hdr);

	return (dev_start());
}

/* generate a tmpfs snapshot representation of an inode described by 'arcn' */
int
tmpfs_wr(ARCHD *arcn)
{
	tmpfs_snap_node_t hdr;
	char *name;

	memset(&hdr, 0, sizeof(hdr));
	name = strrchr(arcn->name, '/');
	strlcpy(hdr.tsn_name, (name == NULL) ? arcn->name : name + 1,
	    sizeof(hdr.tsn_name));

	hdr.tsn_id = idmap_translate(arcn->sb.st_ino);
	if (hdr.tsn_id == 0)
		return (-1);

	hdr.tsn_parent = dcache_parent_id(arcn->name);
	hdr.tsn_uid = arcn->sb.st_uid;
	hdr.tsn_gid = arcn->sb.st_gid;
	hdr.tsn_mode = arcn->sb.st_mode;
	hdr.tsn_flags = tmpfs_snap_flags_hdr(arcn->sb.st_flags);
	hdr.tsn_atime = TMPFS_TS_TO_TST(arcn->sb.st_atim);
	hdr.tsn_mtime = TMPFS_TS_TO_TST(arcn->sb.st_mtim);
	hdr.tsn_ctime = TMPFS_TS_TO_TST(arcn->sb.st_ctim);

	if (hdr_set_type(&hdr, arcn->sb.st_mode) == -1)
		return (-1);

	switch (arcn->type) {
	case PAX_BLK:
	case PAX_CHR:
		hdr.tsn_spec.tsn_dev = arcn->sb.st_rdev;
		break;
	case PAX_REG:
		hdr.tsn_spec.tsn_size = arcn->sb.st_size;
		break;
	case PAX_SLK:
		hdr.tsn_spec.tsn_size = arcn->ln_nlen;
		break;
	case PAX_DIR:
		if (dcache_insert(arcn->name, hdr.tsn_id) == -1)
			return (-1);
		break;
	case PAX_SCK:
	case PAX_FIF:
	case PAX_HLK:
	case PAX_HRG:
		/* do nothing */
		break;
	default:
		paxwarn(1, "unknown pax type %d", arcn->type);
		return (-1);
	}

	TMPFS_SNAP_NODE_HTON(&hdr);
	if (wr_rdbuf((char *)&hdr, sizeof(hdr)) == -1) {
		paxwarn(1, "wr_rdbuf() failed");
		return (-1);
	}
	bytes_written += sizeof(hdr);

	if (arcn->type == PAX_REG) {
		bytes_written += arcn->sb.st_size;
		return (0);	/* file ok, dump contents */
	}

	if (arcn->type == PAX_SLK) {
		if (wr_rdbuf(arcn->ln_name, arcn->ln_nlen) == -1) {
			paxwarn(1, "wr_rdbuf() failed");
			return (-1);
		}
		bytes_written += arcn->ln_nlen;
	}

	return (1);	/* file ok, but don't dump its contents */
}

int
tmpfs_endwr(void)
{
	tmpfs_snap_t tshdr;
	struct dcachenode *dn;
	struct idmapnode *idn;

	while ((dn = SPLAY_ROOT(&dcache)) != NULL) {
		SPLAY_REMOVE(dcache, &dcache, dn);
		free(dn);
	}

	while ((idn = SPLAY_ROOT(&idmap)) != NULL) {
		SPLAY_REMOVE(idmap, &idmap, idn);
		free(idn);
	}

	memset(&tshdr, 0, sizeof(tshdr));
	strlcpy(tshdr.ts_magic, TMPFS_SNAP_MAGIC, sizeof(tshdr.ts_magic));
	tshdr.ts_version = TMPFS_SNAP_VERSION;
	tshdr.ts_snap_sz = bytes_written;
	TMPFS_SNAP_HTON(&tshdr);

	wr_fin();
	if (ar_rev(roundup(bytes_written, 512)) == -1) {
		paxwarn(1, "ar_rev() failed");
		return (-1);
	}
	if (ar_write((char *)&tshdr, sizeof(tshdr)) != sizeof(tshdr)) {
		paxwarn(1, "ar_write() failed");
		return (-1);
	}

	return (0);
}

int
tmpfs_trail(ARCHD *arcn, char *buf, int in_resync, int *cnt)
{
	return (-1);
}
