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
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <uvm/uvm_aobj.h>

#include "tmpfs.h"
#include "tmpfs_snapshot.h"

int
tmpfs_snap_rdwr(struct vnode *vp, enum uio_rw rw, void *ptr, size_t sz,
    uint64_t *off)
{
	int error = 0;
	struct proc *p = curproc;
	int ioflags = IO_UNIT | IO_NODELOCKED;

	if (*off > OFF_MAX || OFF_MAX - *off < sz)
		return (EINVAL);

	error = vn_rdwr(rw, vp, ptr, (int)sz, *off, UIO_SYSSPACE, ioflags,
	    p->p_ucred, NULL, p);

	*off += sz;

	return (error);
}

int
tmpfs_snap_file_io(struct vnode *vp, enum uio_rw rw, const tmpfs_node_t *node,
    uint64_t *off)
{
	vaddr_t va;
	vsize_t sz;
	off_t done;
	int error = 0;
	uvm_flag_t uvmflags;

	KASSERT(node->tn_type == VREG);

	if (rw == UIO_READ)
		uvmflags = UVM_MAPFLAG(UVM_PROT_W, UVM_PROT_W, UVM_INH_NONE,
		    UVM_ADV_SEQUENTIAL, 0);
	else
		uvmflags = UVM_MAPFLAG(UVM_PROT_R, UVM_PROT_R, UVM_INH_NONE,
		    UVM_ADV_SEQUENTIAL, 0);

	for (done = 0; node->tn_size - done > 0; done += sz) {
		sz = node->tn_size - done;
		if (sz > TMPFS_UIO_MAXBYTES)
			sz = TMPFS_UIO_MAXBYTES;
		uao_reference(node->tn_uobj);
		error = uvm_map(kernel_map, &va, round_page(sz), node->tn_uobj,
		    trunc_page(done), 0, uvmflags);
		if (error) {
			uao_detach(node->tn_uobj);
			return (error);
		}
		error = tmpfs_snap_rdwr(vp, rw, (void *)va, sz, off);
		uvm_unmap(kernel_map, va, va + round_page(sz));
		if (error)
			break;
	}

	return (error);
}

int
tmpfs_snap_bmap_lookup(tmpfs_snap_bmap_t *map, ino_t bitnum)
{
	assert(bitnum < map->tsb_entries);

	if (isset(map->tsb_map, bitnum))
		return (1);
	setbit(map->tsb_map, bitnum);
	return (0);
}

void
tmpfs_snap_fill_hdr(tmpfs_snap_node_t *tnhdr, const tmpfs_node_t *node)
{
	tnhdr->tsn_id = node->tn_id;
	tnhdr->tsn_uid = node->tn_uid;
	tnhdr->tsn_gid = node->tn_gid;
	tnhdr->tsn_mode = node->tn_mode;
	tnhdr->tsn_type = tmpfs_snap_type_hdr(node->tn_type);
	tnhdr->tsn_flags = tmpfs_snap_flags_hdr(node->tn_flags);
	/* tnhdr->tsn_btime = TMPFS_TS_TO_TST(node->tn_btime); */
	tnhdr->tsn_atime = TMPFS_TS_TO_TST(node->tn_atime);
	tnhdr->tsn_mtime = TMPFS_TS_TO_TST(node->tn_mtime);
	tnhdr->tsn_ctime = TMPFS_TS_TO_TST(node->tn_ctime);
}

int
tmpfs_snap_dump_hdr(struct vnode *vp, uint64_t *off, tmpfs_snap_node_t *tnhdr)
{
	int error;

	TMPFS_SNAP_NODE_HTON(tnhdr);
	error = tmpfs_snap_rdwr(vp, UIO_WRITE, tnhdr, sizeof(*tnhdr), off);
	TMPFS_SNAP_NODE_NTOH(tnhdr);

	return (error);
}

int
tmpfs_snap_dump_file(struct vnode *vp, uint64_t *off, tmpfs_snap_node_t *tnhdr,
    const tmpfs_dirent_t *td)
{
	int error;

	tnhdr->tsn_spec.tsn_size = td->td_node->tn_size;
	error = tmpfs_snap_dump_hdr(vp, off, tnhdr);

	if (error == 0 && tnhdr->tsn_spec.tsn_size != 0)
		error = tmpfs_snap_file_io(vp, UIO_WRITE, td->td_node, off);

	return (error);
}

int
tmpfs_snap_dump_link(struct vnode *vp, uint64_t *off, tmpfs_snap_node_t *tnhdr,
    const tmpfs_dirent_t *td)
{
	int error;

	KASSERT(td->td_node->tn_size <= MAXPATHLEN);

	tnhdr->tsn_spec.tsn_size = td->td_node->tn_size;
	error = tmpfs_snap_dump_hdr(vp, off, tnhdr);
	if (error == 0)
		error = tmpfs_snap_rdwr(vp, UIO_WRITE,
		    td->td_node->tn_spec.tn_lnk.tn_link, td->td_node->tn_size,
		    off);

	return (error);
}

int
tmpfs_snap_dump_dev(struct vnode *vp, uint64_t *off, tmpfs_snap_node_t *tnhdr,
    const tmpfs_dirent_t *td)
{
	tnhdr->tsn_spec.tsn_dev = td->td_node->tn_spec.tn_dev.tn_rdev;

	return (tmpfs_snap_dump_hdr(vp, off, tnhdr));
}

/*
 * Write the content and meta-data of a directory entry to the snapshot
 * target file.
 */
int
tmpfs_snap_dump_dirent(struct vnode *vp, uint64_t *off,
    tmpfs_snap_node_t *tnhdr, const tmpfs_dirent_t *td,
    tmpfs_snap_bmap_t *id_map)
{
	KASSERT(td->td_namelen <= MAXNAMLEN);
	memcpy(tnhdr->tsn_name, td->td_name, td->td_namelen);

	tmpfs_snap_fill_hdr(tnhdr, td->td_node);

	/* Avoid dumping hardlinks multiple times. */
	if (tmpfs_snap_bmap_lookup(id_map, td->td_node->tn_id))
		return (tmpfs_snap_dump_hdr(vp, off, tnhdr));

	switch (td->td_node->tn_type) {
	case VREG:
		return (tmpfs_snap_dump_file(vp, off, tnhdr, td));
	case VLNK:
		return (tmpfs_snap_dump_link(vp, off, tnhdr, td));
	case VDIR:
	case VFIFO:
	case VSOCK:
		return (tmpfs_snap_dump_hdr(vp, off, tnhdr));
	case VBLK:
	case VCHR:
		return (tmpfs_snap_dump_dev(vp, off, tnhdr, td));
	default:
		printf("%s: don't know how to handle type %d\n", __func__,
		    (int)td->td_node->tn_type);
	}

	return (EOPNOTSUPP);
}

int
tmpfs_snap_dump_root(struct vnode *vp, uint64_t *off, const tmpfs_mount_t *tmp,
    tmpfs_snap_node_t *tnhdr)
{
	KASSERT(tmp->tm_root != NULL);

	tnhdr->tsn_parent = tmp->tm_root->tn_id;
	strlcpy(tnhdr->tsn_name, "/", sizeof(tnhdr->tsn_name));

	tmpfs_snap_fill_hdr(tnhdr, tmp->tm_root);

	return (tmpfs_snap_dump_hdr(vp, off, tnhdr));
}

/*
 * Traverse the file system tree in depth-first order.
 *
 * Before starting the traversal the td and parent variables have to be
 * initialized to NULL. Then the function is to be called repeatedly
 * until returning non-zero. e.g.:
 *
 * tmpfs_dirent_t *td = NULL;
 * tmpfs_node_t *parent = NULL;
 * while (tmpfs_snap_hierwalk(tmp, &td, &parent) == 0)
 *         process_node(td);
 */
int
tmpfs_snap_hierwalk(const tmpfs_mount_t *tmp, const tmpfs_dirent_t **td,
    const tmpfs_node_t **parent)
{
	const tmpfs_node_t *node;
	const tmpfs_dirent_t *next;

	if (*td == NULL) {
		*td = TAILQ_FIRST(&tmp->tm_root->tn_spec.tn_dir.tn_dir);
		*parent = tmp->tm_root;
		return (*td == NULL);
	}

	node = (*td)->td_node;
	if (node->tn_type == VDIR &&
	    (next = TAILQ_FIRST(&node->tn_spec.tn_dir.tn_dir))) {
		*td = next;
		*parent = node;
		return (0);
	} else if ((next = TAILQ_NEXT(*td, td_entries))) {
		*td = next;
		return (0);
	}

	while (*parent != (*parent)->tn_spec.tn_dir.tn_parent) {
		next = TAILQ_NEXT((*parent)->tn_dirent_hint, td_entries);
		if (next) {
			*td = next;
			*parent = (*parent)->tn_spec.tn_dir.tn_parent;
			return (0);
		}
		*parent = (*parent)->tn_spec.tn_dir.tn_parent;
	}

	return (1);
}

/*
 * Write the contents of a mountpoint to a snapshot file.
 */
int
tmpfs_snap_dump(struct mount *mp, const char *path, struct proc *p)
{
	const tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
	const tmpfs_dirent_t *td = NULL;
	const tmpfs_node_t *parent = NULL;
	struct nameidata nd;
	struct vnode *vp;
	tmpfs_snap_t tshdr;		/* tmpfs snapshot header. */
	tmpfs_snap_node_t tnhdr;	/* tmpfs node header. */
	int error;
	off_t off = 0;
	tmpfs_snap_bmap_t id_map;

	id_map.tsb_entries = tmp->tm_highest_inode + 1;
	id_map.tsb_map = mallocarray(howmany(id_map.tsb_entries, NBBY),
	    sizeof(uint8_t), M_TEMP, M_WAITOK | M_CANFAIL | M_ZERO);
	if (id_map.tsb_map == NULL)
		return (ENOMEM);

	NDINIT(&nd, CREATE, FOLLOW, UIO_SYSSPACE, path, p);
	error = vn_open(&nd, O_CREAT | O_EXCL | FWRITE, S_IRUSR);
	if (error) {
		free(id_map.tsb_map, M_TEMP, 0);
		return (error);
	}
	vp = nd.ni_vp;

	/* The snapshot file must reside on a different mountpoint. */
	if (vp->v_mount == mp) {
		error = EINVAL;
		goto out;
	}

	bzero(&tshdr, sizeof(tshdr));
	strlcpy(tshdr.ts_magic, TMPFS_SNAP_MAGIC, sizeof(tshdr.ts_magic));
	tshdr.ts_version = TMPFS_SNAP_VERSION;

	error = tmpfs_snap_rdwr(vp, UIO_WRITE, &tshdr, sizeof(tshdr), &off);
	if (error)
		goto out;

	/* Dump root node. */
	bzero(&tnhdr, sizeof(tnhdr));
	error = tmpfs_snap_dump_root(vp, &off, tmp, &tnhdr);
	if (error)
		goto out;

	/* Dump all other nodes. */
	while (tmpfs_snap_hierwalk(tmp, &td, &parent) == 0) {
		bzero(&tnhdr, sizeof(tnhdr));
		tnhdr.tsn_parent = parent->tn_id;
		error = tmpfs_snap_dump_dirent(vp, &off, &tnhdr, td, &id_map);
		if (error)
			goto out;
	}

	/* Stamp the header with the total of bytes written. */
	tshdr.ts_snap_sz = off;
	TMPFS_SNAP_HTON(&tshdr);
	off = 0;
	error = tmpfs_snap_rdwr(vp, UIO_WRITE, &tshdr, sizeof(tshdr), &off);

out:
	free(id_map.tsb_map, M_TEMP, 0);
	VOP_UNLOCK(vp, 0);
	vn_close(vp, FWRITE, p->p_ucred, p);

	return (error);
}

void
tmpfs_snap_load_cleanup(tmpfs_mount_t *tmp)
{
	tmpfs_node_t *node, *nnode;
	tmpfs_dirent_t *de, *nde;

	LIST_FOREACH(node, &tmp->tm_nodes, tn_entries) {
		if (node->tn_type != VDIR)
			continue;

		TAILQ_FOREACH_SAFE(de, &node->tn_spec.tn_dir.tn_dir,
		    td_entries, nde) {
			tmpfs_dir_detach(node, de);
			tmpfs_free_dirent(tmp, de);
		}
	}

	LIST_FOREACH_SAFE(node, &tmp->tm_nodes, tn_entries, nnode) {
		tmpfs_free_node(tmp, node);
	}
}

void
tmpfs_snap_find_node(const tmpfs_mount_t *tmp, ino_t id, tmpfs_node_t **nodepp)
{
	tmpfs_node_t *node;

	*nodepp = NULL;

	LIST_FOREACH(node, &tmp->tm_nodes, tn_entries) {
		if (node->tn_id == id) {
			*nodepp = node;
			break;
		}
	}
}

int
tmpfs_snap_find_parent(const tmpfs_mount_t *tmp, const tmpfs_snap_node_t *tnhdr,
    tmpfs_node_t **dnode)
{
	if (tnhdr->tsn_parent == 2) {
		*dnode = tmp->tm_root;
		return (0);
	}

	tmpfs_snap_find_node(tmp, tnhdr->tsn_parent, dnode);
	if (*dnode == NULL || (*dnode)->tn_type != VDIR)
		return (EFTYPE);

	return (0);
}

int
tmpfs_snap_alloc_node(tmpfs_mount_t *tmp, const tmpfs_snap_node_t *tnhdr,
    char *target, dev_t rdev, tmpfs_node_t **node)
{
	int error;

	if (tnhdr->tsn_id == tnhdr->tsn_parent) {
		if (tnhdr->tsn_id != 2)
			return (EFTYPE);
		*node = tmp->tm_root;
		return (0);
	}

	error = tmpfs_alloc_node(tmp, tmpfs_snap_type_node(tnhdr->tsn_type),
	    tnhdr->tsn_uid, tnhdr->tsn_gid, tnhdr->tsn_mode, target, rdev,
	    node);
	if (error)
		return (error);

	(*node)->tn_id = tnhdr->tsn_id;
	if (tnhdr->tsn_id > tmp->tm_highest_inode)
		tmp->tm_highest_inode = tnhdr->tsn_id;

	return (0);
}

int
tmpfs_snap_attach_node(tmpfs_mount_t *tmp, tmpfs_snap_node_t *tnhdr,
    tmpfs_node_t *node)
{
	tmpfs_node_t *dnode = NULL;
	tmpfs_dirent_t *de;
	int error;

	if (node == tmp->tm_root)
		return (0);	/* No need to attach root. */

	tnhdr->tsn_name[sizeof(tnhdr->tsn_name) - 1] = '\0';
	error = tmpfs_alloc_dirent(tmp, tnhdr->tsn_name,
	    strlen(tnhdr->tsn_name), &de);
	if (error) {
		tmpfs_free_node(tmp, node);
		return (error);
	}

	error = tmpfs_snap_find_parent(tmp, tnhdr, &dnode);
	if (error) {
		tmpfs_free_node(tmp, node);
		return (error);
	}

	error = tmpfs_snap_dir_attach(dnode, de, node);
	if (error)
		goto out;

	return (0);
out:
	tmpfs_free_dirent(tmp, de);
	tmpfs_free_node(tmp, node);

	return (error);
}

int
tmpfs_snap_load_hdr(tmpfs_mount_t *tmp, tmpfs_node_t *node,
    tmpfs_snap_node_t *tnhdr, char *target, dev_t dev)
{
	int error;

	if (node == NULL) {
		error = tmpfs_snap_alloc_node(tmp, tnhdr, target, dev, &node);
		if (error)
			return (error);
	}

	error = tmpfs_snap_attach_node(tmp, tnhdr, node);
	if (error)
		return (error);

	node->tn_flags = tmpfs_snap_flags_node(tnhdr->tsn_flags);
	/* node->tn_btime = TMPFS_TST_TO_TS(tnhdr->tsn_btime); */
	node->tn_atime = TMPFS_TST_TO_TS(tnhdr->tsn_atime);
	node->tn_mtime = TMPFS_TST_TO_TS(tnhdr->tsn_mtime);
	node->tn_ctime = TMPFS_TST_TO_TS(tnhdr->tsn_ctime);

	return (0);
}

int
tmpfs_snap_load_file(struct vnode *vp, uint64_t *off, tmpfs_mount_t *tmp,
    tmpfs_snap_node_t *tnhdr)
{
	tmpfs_node_t *node = NULL;
	int error;

	tmpfs_snap_find_node(tmp, tnhdr->tsn_id, &node);
	if (node) {
		if (node->tn_type != VREG)
			return (EFTYPE);
	} else {
		error = tmpfs_snap_alloc_node(tmp, tnhdr, NULL, NODEV, &node);
		if (error)
			return (error);
	}

	if (tnhdr->tsn_spec.tsn_size) {
		error = tmpfs_snap_node_setsize(tmp, node,
		    tnhdr->tsn_spec.tsn_size);
		if (error)
			goto out;
		error = tmpfs_snap_file_io(vp, UIO_READ, node, off);
		if (error)
			goto out;
	}

	return (tmpfs_snap_load_hdr(tmp, node, tnhdr, NULL, NODEV));

out:
	tmpfs_free_node(tmp, node);

	return (error);
}

int
tmpfs_snap_load_link(struct vnode *vp, uint64_t *off, tmpfs_mount_t *tmp,
    tmpfs_snap_node_t *tnhdr)
{
	char target[MAXPATHLEN + 1];
	int error;

	if (tnhdr->tsn_spec.tsn_size > MAXPATHLEN)
		return (EFTYPE);

	error = tmpfs_snap_rdwr(vp, UIO_READ, target, tnhdr->tsn_spec.tsn_size,
	    off);
	if (error)
		return (error);

	target[tnhdr->tsn_spec.tsn_size] = '\0';

	return (tmpfs_snap_load_hdr(tmp, NULL, tnhdr, target, NODEV));
}

/*
 * Load one node from the snapshot file and store it in the file system.
 */
int
tmpfs_snap_load_node(struct vnode *vp, struct mount *mp, uint64_t *off)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
	tmpfs_snap_node_t hdr;
	dev_t dev = NODEV;
	int vtype, error;

	error = tmpfs_snap_rdwr(vp, UIO_READ, &hdr, sizeof(hdr), off);
	if (error)
		return (error);

	TMPFS_SNAP_NODE_NTOH(&hdr);
	if (hdr.tsn_id < 2)
		return (EFTYPE);

	vtype = tmpfs_snap_type_node(hdr.tsn_type);
	switch (vtype) {
	case VREG:
		return (tmpfs_snap_load_file(vp, off, tmp, &hdr));
	case VLNK:
		return (tmpfs_snap_load_link(vp, off, tmp, &hdr));
	case VBLK:
	case VCHR:
		dev = hdr.tsn_spec.tsn_dev;
		/* FALLTHROUGH */
	case VDIR:
	case VFIFO:
	case VSOCK:
		return (tmpfs_snap_load_hdr(tmp, NULL, &hdr, NULL, dev));
	default:
		printf("%s: don't know how to handle type %d\n", __func__,
		    vtype);
	}

	return (EFTYPE);
}

int
tmpfs_snap_load_vnode(struct mount *mp, struct vnode *vp, struct proc *p)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
	tmpfs_snap_t hdr;
	uint64_t off = 0;
	int error;

	KASSERT(tmp->tm_highest_inode == 2);

	error = tmpfs_snap_rdwr(vp, UIO_READ, &hdr, sizeof(hdr), &off);
	if (error)
		return (error);

	if (strcmp(hdr.ts_magic, TMPFS_SNAP_MAGIC) ||
	    hdr.ts_version != TMPFS_SNAP_VERSION)
		return (EFTYPE);

	TMPFS_SNAP_NTOH(&hdr);

	if (hdr.ts_snap_sz > OFF_MAX)
		return (EINVAL);

	off = sizeof(hdr);
	while (off < hdr.ts_snap_sz) {
		error = tmpfs_snap_load_node(vp, mp, &off);
		if (error) {
			tmpfs_snap_load_cleanup(tmp);
			break;
		}
	}

	return (error);
}

/*
 * Load a snapshot file and populate a mountpoint with its contents.
 */
int
tmpfs_snap_load(struct mount *mp, const char *path, struct proc *p)
{
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path, p);

	error = vn_open(&nd, FREAD, 0);
	if (error == 0) {
		error = tmpfs_snap_load_vnode(mp, nd.ni_vp, p);
		VOP_UNLOCK(nd.ni_vp, 0);
		vn_close(nd.ni_vp, FREAD, p->p_ucred, p);
	}

	return (error);
}

/*
 * Attach node into the parent directory.
 *
 * Similar to tmpfs_dir_attach, but do not update timestamps and fire kqueue
 * event notifications, so timestamps retrieved from the snapshot file are
 * retained.
 */
int
tmpfs_snap_dir_attach(tmpfs_node_t *dnode, tmpfs_dirent_t *de,
    tmpfs_node_t *node)
{
	if (node->tn_links >= LINK_MAX)
		return (EMLINK);

	if (TMPFS_DIRSEQ_FULL(dnode))
		return (ENOSPC);

	if (node->tn_type == VDIR && dnode->tn_links >= LINK_MAX)
		return (EMLINK);

	de->td_seq = tmpfs_dir_getseq(dnode, de);
	de->td_node = node;

	node->tn_links++;
	node->tn_dirent_hint = de;

	TAILQ_INSERT_TAIL(&dnode->tn_spec.tn_dir.tn_dir, de, td_entries);
	dnode->tn_size += sizeof(*de);
	if (dnode->tn_vnode)
		uvm_vnp_setsize(dnode->tn_vnode, dnode->tn_size);

	if (node->tn_type == VDIR) {
		node->tn_spec.tn_dir.tn_parent = dnode;
		dnode->tn_links++;
		TMPFS_VALIDATE_DIR(node);
	}

	return (0);
}

/*
 * Set the size of a regular file.
 */
int
tmpfs_snap_node_setsize(tmpfs_mount_t *tmp, tmpfs_node_t *node, uint64_t size)
{
	size_t pages, bytes;

	KASSERT(node->tn_type == VREG);
	KASSERT(!tmpfs_uio_cached(node));
	KASSERT(size != 0);
	KASSERT(node->tn_size == 0);
	KASSERT(node->tn_spec.tn_reg.tn_aobj_pages == 0);

	if (size > TMPFS_MAX_FILESIZE)
		return (EFTYPE);

	bytes = round_page(size);
	if (tmpfs_mem_incr(tmp, bytes) == 0)
		return (ENOSPC);

	pages = round_page(size) >> PAGE_SHIFT;
	if (uao_setsize(node->tn_uobj, (int)pages) != 0) {
		tmpfs_mem_decr(tmp, bytes);
		return (ENOSPC);
	}

	node->tn_size = size;
	node->tn_spec.tn_reg.tn_aobj_pages = pages;

	return (0);
}
