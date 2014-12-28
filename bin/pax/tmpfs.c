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

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <util.h>

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

static SPLAY_HEAD(icache, icachenode)	 icache = SPLAY_INITIALIZER(&icache);
static SPLAY_HEAD(dcache, dcachenode)	 dcache = SPLAY_INITIALIZER(&dcache);
static SPLAY_HEAD(idmap, idmapnode)	 idmap = SPLAY_INITIALIZER(&idmap);
static off_t				 bytes_to_read;
static off_t				 bytes_written;
static uint64_t				 from_highest_inode;
static uint64_t				 to_highest_inode;
static const char			*device_manifest;

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
static int			 consume(off_t);

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

static int
dcache_id(const char *path, uint64_t *id)
{
	struct dcachenode find, *node;

	strlcpy(find.path, normpath(path), sizeof(find.path));

	node = SPLAY_FIND(dcache, &dcache, &find);
	if (node == NULL)
		return (-1);

	if (id)
		*id = node->id;

	return (0);
}

static uint64_t
dcache_parent_id(const char *path)
{
	char parent_path[PAXPATHLEN + 1], *slash;
	uint64_t parent_id;

	strlcpy(parent_path, normpath(path), sizeof(parent_path));
	slash = strrchr(parent_path, '/');
	if (slash == NULL)
		return (ROOT_ID);
	*slash = '\0';

	if (dcache_id(parent_path, &parent_id) < 0)
		return (ROOT_ID);

	return (parent_id);
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
		node->to = ++to_highest_inode;
		if (node->to == 0) {
			paxwarn(1,
			    "couldn't allocate inode number; too many files");
			free(node);
			return (0);
		}

		if (from > from_highest_inode)
			from_highest_inode = from;

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

static int
consume(off_t n)
{
	if (n > bytes_to_read) {
		paxwarn(1, "malformed tmpfs snapshot; invalid ts_snap_sz");
		return (-1);
	}
	bytes_to_read -= n;
	return (0);
}

int
tmpfs_id(char *blk, int size)
{
	tmpfs_snap_t *hdr;

	if (size < 0 || (size_t)size < sizeof(*hdr))
		return (-1);

	hdr = (tmpfs_snap_t *)blk;

	if (strcmp(hdr->ts_magic, TMPFS_SNAP_MAGIC) ||
	    hdr->ts_version != TMPFS_SNAP_VERSION)
		return (-1);

	force_one_volume = 1;

	TMPFS_SNAP_NTOH(hdr);
	bytes_to_read = hdr->ts_snap_sz;

	return (0);
}

int
tmpfs_strd(void)
{
	if (consume(sizeof(tmpfs_snap_t)) == -1)
		return (-1);
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

	if (consume(sizeof(*hdr)) == -1)
		return (-1);
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
		if (consume(hdr->tsn_spec.tsn_size) == -1)
			return (-1);
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
		if (arcn->sb.st_size < 0 ||
		    (size_t)arcn->sb.st_size >= sizeof(arcn->ln_name)) {
			paxwarn(1, "symlink too long (%llu bytes)",
			    hdr->tsn_spec.tsn_size);
			return (-1);
		}
		arcn->ln_nlen = (int)arcn->sb.st_size;
		if (rd_wrbuf(arcn->ln_name, arcn->ln_nlen) != arcn->ln_nlen) {
			paxwarn(1, "rd_wrbuf() failed");
			return (-1);
		}
		if (consume(hdr->tsn_spec.tsn_size) == -1)
			return (-1);
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

	to_highest_inode = ROOT_ID;

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

static int
check_perm(char v, char c, mode_t *mode, mode_t mask)
{
	if (v == c)
		*mode |= mask;
	else if (v != '-')
		return (-1);

	return (0);
}

static int
parse_perm(const char **line, mode_t *mode, uid_t *uid, gid_t *gid)
{
	char perm[17], user[17], group[17];
	struct passwd *pwd;
	struct group *grp;
	int n = -1;

	if (sscanf(*line, "%16s %*u %16s %16s %n", perm, user, group,
	    &n) != 3 || n < 0)
		return (-1);
	*line += (size_t)n;

	perm[sizeof(perm) - 1] = '\0';
	if (strlen(perm) != 10)
		return (-1);

	*mode = 0;

	switch (perm[0]) {
	case 'b':
		*mode |= S_IFBLK;
		break;
	case 'c':
		*mode |= S_IFCHR;
		break;
	case 'd':
		*mode |= S_IFDIR;
		break;
	case 'l':
		*mode |= S_IFLNK;
		break;
	default:
		paxwarn(1, "unknown file type '%c'", perm[0]);
		return (-1);
	}

	if (check_perm(perm[1], 'r', mode, S_IRUSR) < 0 ||
	    check_perm(perm[2], 'w', mode, S_IWUSR) < 0 ||
	    check_perm(perm[3], 'x', mode, S_IXUSR) < 0 ||
	    check_perm(perm[4], 'r', mode, S_IRGRP) < 0 ||
	    check_perm(perm[5], 'w', mode, S_IWGRP) < 0 ||
	    check_perm(perm[6], 'x', mode, S_IXGRP) < 0 ||
	    check_perm(perm[7], 'r', mode, S_IROTH) < 0 ||
	    check_perm(perm[8], 'w', mode, S_IWOTH) < 0 ||
	    check_perm(perm[9], 'x', mode, S_IXOTH) < 0)
		return (-1);

	user[sizeof(user) - 1] = '\0';
	if ((pwd = getpwnam(user)) == NULL) {
		paxwarn(1, "no such user");
		return (-1);
	}
	*uid = pwd->pw_uid;

	group[sizeof(group) - 1] = '\0';
	if ((grp = getgrnam(group)) == NULL) {
		paxwarn(1, "no such group");
		return (-1);
	}
	*gid = grp->gr_gid;

	return (0);
}

static int
parse_date(struct stat *st, const char **line)
{
	struct tm tm;
	time_t t;

	if ((*line = strptime(*line, "%b %d %T %Y", &tm)) == NULL)
		return (-1);

	t = mktime(&tm);
	if (t == (time_t)-1)
		return (-1);

	st->st_atim.tv_sec = st->st_mtim.tv_sec = st->st_ctim.tv_sec = t;

	return (0);
}

static int
parse_device(struct stat *st, const char **line)
{
	unsigned int major, minor;
	int n = -1;

	if (sscanf(*line, "%u, %u %n", &major, &minor, &n) != 2 || n < 0)
		return (-1);
	*line += (size_t)n;

	st->st_rdev = makedev(major, minor);

	return (0);
}

static int
skip_size(const char **line)
{
	int n = -1;

	if (sscanf(*line, "%*u %n", &n) != 0 || n < 0)
		return (-1);
	*line += (size_t)n;

	return (0);
}

static int
parse_manifest_entry(const char *parent, const char *line)
{
	ARCHD arcn;
	struct stat *st = &arcn.sb;
	char path[64];
	int n, r = -1;

	if (strlen(parent) < 1 || strlen(line) < 1) {
		paxwarn(1, "don't know where to place manifest entry");
		return (-1);
	}

	memset(&arcn, 0, sizeof(arcn));

	/* this is just so we can use tmpfs_wr() as it is. */
	if (from_highest_inode == UINT64_MAX) {
		paxwarn(1, "out of inodes");
		return (-1);
	}
	arcn.sb.st_ino = ++from_highest_inode;

	if (parse_perm(&line, &st->st_mode, &st->st_uid, &st->st_gid) < 0)
		return (-1);

	if (S_ISBLK(st->st_mode)) {
		arcn.type = PAX_BLK;
		r = parse_device(st, &line);
	} else if (S_ISCHR(st->st_mode)) {
		arcn.type = PAX_CHR;
		r = parse_device(st, &line);
	} else if (S_ISDIR(st->st_mode)) {
		arcn.type = PAX_DIR;
		r = skip_size(&line);
	} else if (S_ISLNK(st->st_mode)) {
		arcn.type = PAX_SLK;
		r = skip_size(&line);
	}

	if (r < 0 || parse_date(st, &line) < 0)
		return (-1);

	n = -1;
	if (arcn.type == PAX_SLK) {
		_Static_assert(sizeof(arcn.ln_name) >= 64,
		    "this code assumes sizeof(arcn.ln_name) >= 64");
		if (sscanf(line, "%63s -> %63s %n", path, arcn.ln_name,
		    &n) != 2 || n < 0)
			return (-1);
		arcn.ln_name[sizeof(arcn.ln_name) - 1] = '\0';
		arcn.ln_nlen = strlen(arcn.ln_name);
	} else if (sscanf(line, "%63s %n", path, &n) != 1 || n < 0)
		return (-1);

	line += (size_t)n;

	path[sizeof(path) - 1] = '\0';
	if (strchr(path, '/'))
		return (-1);

	n = snprintf(arcn.name, sizeof(arcn.name), "%s/%s", parent, path);
	if (n < 0 || (size_t)n >= sizeof(arcn.name))
		return (-1);
	arcn.nlen = n;

	return (tmpfs_wr(&arcn));
}

static int
populate_device_manifest(const char *manifest)
{
	FILE *f;
	char *l;
	int n, ok = -1;
	size_t ln = 0;
	char path[64], cwd[64] = "";

	if ((f = fopen(manifest, "r")) == NULL) {
		syswarn(1, errno, "fopen %s", manifest);
		return (-1);
	}

	if (setpassent(1) == 0)
		syswarn(1, errno, "setpassent");
	if (setgroupent(1) == 0)
		syswarn(1, errno, "setgroupent");

	while ((l = fparseln(f, NULL, &ln, NULL, 0)) != NULL) {
		n = -1;
		if (sscanf(l, "%63s %n", path, &n) == 1 && n > 0 &&
		    (size_t)n == strlen(l)) {
			path[sizeof(path) - 1] = '\0';
			if (strlen(path) < 1 || path[0] != '/' ||
			    path[strlen(path) - 1] != ':') {
				paxwarn(1, "full path required");
				goto out;
			}
			path[strlen(path) - 1] = '\0'; /* clip */
			if (dcache_id(path, NULL) < 0 && strcmp(path, "/")) {
				paxwarn(1, "directory not found: %s", path);
				goto out;
			}
			strlcpy(cwd, path, sizeof(cwd));
		} else if (strlen(l) && parse_manifest_entry(cwd, l) < 0)
			goto out;
		free(l);
	}

	ok = 0;
out:
	free(l);
	fclose(f);
	endpwent();
	endgrent();

	if (ok < 0)
		paxwarn(1, "failed to parse device manifest line %zu", ln);

	return (ok);
}

int
tmpfs_endwr(void)
{
	tmpfs_snap_t tshdr;
	struct dcachenode *dn;
	struct idmapnode *idn;

	if (device_manifest != NULL) {
		if (populate_device_manifest(device_manifest) < 0) {
			paxwarn(1, "failed to populate device manifest");
			return (-1);
		}
		free((void *)device_manifest);
	}

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
	return (bytes_to_read > 0 ? -1 : 0);
}

int
tmpfs_opt(void)
{
	OPLIST *opt;

	if ((opt = opt_next()) == NULL)
		return (0);

	if (strcmp(opt->name, "devmanifest") || act != ARCHIVE) {
		paxwarn(1, "unknown option %s", opt->name);
		return (-1);
	}

	device_manifest = strdup(opt->value);
	if (device_manifest == NULL) {
		syswarn(1, errno, "strdup");
		return (-1);
	}

	if ((opt = opt_next()) != NULL) {
		if (strcmp(opt->name, "devmanifest") == 0)
			paxwarn(1, "devmanifest may be specified only once");
		else
			paxwarn(1, "unknown option %s", opt->name);
		return (1);
	}

	return (0);
}
