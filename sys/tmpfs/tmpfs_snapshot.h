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

#ifndef _TMPFS_TMPFS_VNOPS_H_
#define _TMPFS_TMPFS_VNOPS_H_

#define TMPFS_SNAP_MAGIC	"tmpfs\n"
#define TMPFS_SNAP_VERSION	1

typedef struct __attribute__((packed)) {
	char		ts_magic[7];
	uint8_t		ts_version;
	uint64_t	ts_snap_sz;
} tmpfs_snap_t;

typedef struct __attribute__((packed)) {
	int64_t	tst_sec;
	int32_t	tst_nsec;
} tmpfs_snap_timespec_t;
#define TMPFS_TS_TO_TST(ts)	\
	((tmpfs_snap_timespec_t){ (ts).tv_sec, (int32_t)(ts).tv_nsec })
#define TMPFS_TST_TO_TS(tst)	\
	((struct timespec){ (tst).tst_sec, (tst).tst_nsec })

typedef struct __attribute__((packed)) {
	char		tsn_name[MAXNAMLEN + 1];
	uint64_t	tsn_id;
	uint64_t	tsn_parent;
	uint32_t	tsn_uid;
	uint32_t	tsn_gid;
	uint32_t	tsn_mode;

	uint8_t		tsn_type;
#define TMPFS_SNAP_NTYPE_REG	0x00
#define TMPFS_SNAP_NTYPE_DIR	0x01
#define TMPFS_SNAP_NTYPE_BLK	0x02
#define TMPFS_SNAP_NTYPE_CHR	0x03
#define TMPFS_SNAP_NTYPE_LNK	0x04
#define TMPFS_SNAP_NTYPE_SOCK	0x05
#define TMPFS_SNAP_NTYPE_FIFO	0x06

	uint8_t		tsn_flags;
#define TMPFS_SNAP_UF_NODUMP	0x01
#define TMPFS_SNAP_UF_IMMUTABLE	0x02
#define TMPFS_SNAP_UF_APPEND	0x04
#define TMPFS_SNAP_UF_OPAQUE	0x08
#define TMPFS_SNAP_SF_ARCHIVED	0x10
#define TMPFS_SNAP_SF_IMMUTABLE	0x20
#define TMPFS_SNAP_SF_APPEND	0x40

	tmpfs_snap_timespec_t	tsn_atime;
	tmpfs_snap_timespec_t	tsn_mtime;
	tmpfs_snap_timespec_t	tsn_ctime;

	union __attribute__((packed)) {
		uint64_t	tsn_size;
		int32_t		tsn_dev;
	} tsn_spec;
} tmpfs_snap_node_t;


#ifdef _KERNEL

typedef struct {
	uint64_t	 tsb_entries;
	uint8_t		*tsb_map;
} tmpfs_snap_bmap_t;

#endif

#if BYTE_ORDER == BIG_ENDIAN
#define TMPFS_SNAP_HTON(x)	do {} while (0)
#define TMPFS_SNAP_NTOH(x)	do {} while (0)
#define TMPFS_SNAP_NODE_HTON(x)	do {} while (0)
#define TMPFS_SNAP_NODE_NTOH(x)	do {} while (0)
#else
#define TMPFS_SNAP_HTON(x)	tmpfs_snap_bswap(x)
#define TMPFS_SNAP_NTOH(x)	tmpfs_snap_bswap(x)
#define TMPFS_SNAP_NODE_HTON(x)	tmpfs_snap_node_bswap(x)
#define TMPFS_SNAP_NODE_NTOH(x)	tmpfs_snap_node_bswap(x)
#endif

#ifdef _KERNEL

int	tmpfs_snap_rdwr(struct vnode *, enum uio_rw, void *, size_t,
	    uint64_t *);
int	tmpfs_snap_file_io(struct vnode *, enum uio_rw, const tmpfs_node_t *,
	    uint64_t *);

/*
 * Bitmap manipulation functions. Used to keep track of visited inodes for
 * hardlink handling.
 */

int	tmpfs_snap_bmap_lookup(tmpfs_snap_bmap_t *, ino_t);

/*
 * Functions to dump (generate) a snapshot.
 */

void	tmpfs_snap_fill_hdr(tmpfs_snap_node_t *, const tmpfs_node_t *);
int	tmpfs_snap_dump_hdr(struct vnode *, uint64_t *, tmpfs_snap_node_t *);
int	tmpfs_snap_dump_file(struct vnode *, uint64_t *, tmpfs_snap_node_t *,
	    const tmpfs_dirent_t *);
int	tmpfs_snap_dump_link(struct vnode *, uint64_t *, tmpfs_snap_node_t *,
	    const tmpfs_dirent_t *);
int	tmpfs_snap_dump_dev(struct vnode *, uint64_t *, tmpfs_snap_node_t *,
	    const tmpfs_dirent_t *);
int	tmpfs_snap_dump_dirent(struct vnode *, uint64_t *, tmpfs_snap_node_t *,
	    const tmpfs_dirent_t *, tmpfs_snap_bmap_t *);
int	tmpfs_snap_dump_root(struct vnode *, uint64_t *, const tmpfs_mount_t *,
	    tmpfs_snap_node_t *);

/*
 * Functions to load a snapshot.
 */

void	tmpfs_snap_find_node(const tmpfs_mount_t *, ino_t, tmpfs_node_t **);
void	tmpfs_snap_load_cleanup(tmpfs_mount_t *);
int	tmpfs_snap_find_parent(const tmpfs_mount_t *, const tmpfs_snap_node_t *,
	    tmpfs_node_t **);
int	tmpfs_snap_load_hdr(tmpfs_mount_t *, tmpfs_node_t *,
	    tmpfs_snap_node_t *, char *, dev_t);
int	tmpfs_snap_load_file(struct vnode *, uint64_t *, tmpfs_mount_t *,
	    tmpfs_snap_node_t *);
int	tmpfs_snap_load_link(struct vnode *, uint64_t *, tmpfs_mount_t *,
	    tmpfs_snap_node_t *);
int	tmpfs_snap_alloc_node(tmpfs_mount_t *, const tmpfs_snap_node_t *,
	    char *, dev_t, tmpfs_node_t **);
int	tmpfs_snap_attach_node(tmpfs_mount_t *, tmpfs_snap_node_t *,
	    tmpfs_node_t *);
int	tmpfs_snap_load_node(struct vnode *, struct mount *, uint64_t *);
int	tmpfs_snap_hierwalk(const tmpfs_mount_t *, const tmpfs_dirent_t **,
	    const tmpfs_node_t **);
int	tmpfs_snap_dir_attach(tmpfs_node_t *, tmpfs_dirent_t *, tmpfs_node_t *);
int	tmpfs_snap_node_setsize(tmpfs_mount_t *, tmpfs_node_t *, uint64_t);

/*
 * Auxiliary functions. For a stable binary format the file flags and the vtype
 * enum need to be translated.
 */

static __inline uint8_t
tmpfs_snap_type_hdr(enum vtype tn_type)
{
	switch (tn_type) {
	case VREG:
		return (TMPFS_SNAP_NTYPE_REG);
	case VDIR:
		return (TMPFS_SNAP_NTYPE_DIR);
	case VBLK:
		return (TMPFS_SNAP_NTYPE_BLK);
	case VCHR:
		return (TMPFS_SNAP_NTYPE_CHR);
	case VLNK:
		return (TMPFS_SNAP_NTYPE_LNK);
	case VSOCK:
		return (TMPFS_SNAP_NTYPE_SOCK);
	case VFIFO:
		return (TMPFS_SNAP_NTYPE_FIFO);
	default:
		panic("%s: unknown type %d", __func__, tn_type);
	}
	/* NOTREACHED */
}

static __inline int
tmpfs_snap_type_node(uint8_t tsn_type)
{
	switch (tsn_type) {
	case TMPFS_SNAP_NTYPE_REG:
		return (VREG);
	case TMPFS_SNAP_NTYPE_DIR:
		return (VDIR);
	case TMPFS_SNAP_NTYPE_BLK:
		return (VBLK);
	case TMPFS_SNAP_NTYPE_CHR:
		return (VCHR);
	case TMPFS_SNAP_NTYPE_LNK:
		return (VLNK);
	case TMPFS_SNAP_NTYPE_SOCK:
		return (VSOCK);
	case TMPFS_SNAP_NTYPE_FIFO:
		return (VFIFO);
	}

	return (-1);
}

#endif  /* _KERNEL */

static __inline uint8_t
tmpfs_snap_flags_hdr(uint32_t tn_flags)
{
	uint8_t tsn_flags = 0;

	if (tn_flags & UF_NODUMP)
		tsn_flags |= TMPFS_SNAP_UF_NODUMP;
	if (tn_flags & UF_IMMUTABLE)
		tsn_flags |= TMPFS_SNAP_UF_IMMUTABLE;
	if (tn_flags & UF_APPEND)
		tsn_flags |= TMPFS_SNAP_UF_APPEND;
	if (tn_flags & UF_OPAQUE)
		tsn_flags |= TMPFS_SNAP_UF_OPAQUE;
	if (tn_flags & SF_ARCHIVED)
		tsn_flags |= TMPFS_SNAP_SF_ARCHIVED;
	if (tn_flags & SF_IMMUTABLE)
		tsn_flags |= TMPFS_SNAP_SF_IMMUTABLE;
	if (tn_flags & SF_APPEND)
		tsn_flags |= TMPFS_SNAP_SF_APPEND;

	return (tsn_flags);
}

static __inline uint32_t
tmpfs_snap_flags_node(uint8_t tsn_flags)
{
	uint32_t tn_flags = 0;

	if (tsn_flags & TMPFS_SNAP_UF_NODUMP)
		tn_flags |= UF_NODUMP;
	if (tsn_flags & TMPFS_SNAP_UF_IMMUTABLE)
		tn_flags |= UF_IMMUTABLE;
	if (tsn_flags & TMPFS_SNAP_UF_APPEND)
		tn_flags |= UF_APPEND;
	if (tsn_flags & TMPFS_SNAP_UF_OPAQUE)
		tn_flags |= UF_OPAQUE;
	if (tsn_flags & TMPFS_SNAP_SF_ARCHIVED)
		tn_flags |= SF_ARCHIVED;
	if (tsn_flags & TMPFS_SNAP_SF_IMMUTABLE)
		tn_flags |= SF_IMMUTABLE;
	if (tsn_flags & TMPFS_SNAP_SF_APPEND)
		tn_flags |= SF_APPEND;

	return (tn_flags);
}

/*
 * All values contained by tmpfs_snap_t and tmpfs_snap_node_t are
 * stored in network byte order (big endian). This allows us to exchange
 * snapshot files between different architectures.
 */

#if BYTE_ORDER != BIG_ENDIAN
static __inline void
tmpfs_snap_bswap(tmpfs_snap_t *hdr)
{
	hdr->ts_snap_sz = swap64(hdr->ts_snap_sz);
}

static __inline void
tmpfs_snap_node_bswap(tmpfs_snap_node_t *hdr)
{
	hdr->tsn_id = swap64(hdr->tsn_id);
	hdr->tsn_parent = swap64(hdr->tsn_parent);
	hdr->tsn_uid = swap32(hdr->tsn_uid);
	hdr->tsn_gid = swap32(hdr->tsn_gid);
	hdr->tsn_mode = swap32(hdr->tsn_mode);
	hdr->tsn_atime.tst_sec = swap64(hdr->tsn_atime.tst_sec);
	hdr->tsn_atime.tst_nsec = swap32(hdr->tsn_atime.tst_nsec);
	hdr->tsn_mtime.tst_sec = swap64(hdr->tsn_mtime.tst_sec);
	hdr->tsn_mtime.tst_nsec = swap32(hdr->tsn_mtime.tst_nsec);
	hdr->tsn_ctime.tst_sec = swap64(hdr->tsn_ctime.tst_sec);
	hdr->tsn_ctime.tst_nsec = swap32(hdr->tsn_ctime.tst_nsec);

	switch (hdr->tsn_type) {
	case TMPFS_SNAP_NTYPE_BLK:
	case TMPFS_SNAP_NTYPE_CHR:
		hdr->tsn_spec.tsn_dev = swap32(hdr->tsn_spec.tsn_dev);
		break;
	case TMPFS_SNAP_NTYPE_LNK:
	case TMPFS_SNAP_NTYPE_REG:
		hdr->tsn_spec.tsn_size = swap64(hdr->tsn_spec.tsn_size);
		break;
	default:
		/* nothing to do */
		(void)0;
	}
}
#endif /* BYTE_ORDER != BIG_ENDIAN */

#endif /* _TMPFS_TMPFS_SNAPSHOT_H_ */
