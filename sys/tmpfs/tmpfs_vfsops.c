/*	$NetBSD: tmpfs_vfsops.c,v 1.53 2013/11/08 15:44:23 rmind Exp $	*/

/*
 * Copyright (c) 2005, 2006, 2007 The NetBSD Foundation, Inc.
 * Copyright (c) 2013, 2014 Pedro Martelletto
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Efficient memory file system.
 *
 * tmpfs is a file system that uses NetBSD's virtual memory sub-system
 * (the well-known UVM) to store file data and metadata in an efficient
 * way.  This means that it does not follow the structure of an on-disk
 * file system because it simply does not need to.  Instead, it uses
 * memory-specific data structures and algorithms to automatically
 * allocate and release resources.
 */

#if 0
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_vfsops.c,v 1.53 2013/11/08 15:44:23 rmind Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <tmpfs/tmpfs.h>

/* MODULE(MODULE_CLASS_VFS, tmpfs, NULL); */

struct pool	tmpfs_dirent_pool;
struct pool	tmpfs_node_pool;

int	tmpfs_mountfs(struct mount *, const char *, struct vnode *,
	    struct tmpfs_args *, char *, struct proc *);
int	tmpfs_mountroot(void);
int	tmpfs_mount(struct mount *, const char *, void *, struct nameidata *,
	    struct proc *);
int	tmpfs_start(struct mount *, int, struct proc *);
int	tmpfs_unmount(struct mount *, int, struct proc *);
int	tmpfs_root(struct mount *, struct vnode **);
int	tmpfs_vget(struct mount *, ino_t, struct vnode **);
int	tmpfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int	tmpfs_vptofh(struct vnode *, struct fid *);
int	tmpfs_statfs(struct mount *, struct statfs *, struct proc *);
int	tmpfs_sync(struct mount *, int, struct ucred *, struct proc *);
int	tmpfs_init(struct vfsconf *);
int	tmpfs_mount_update(struct mount *, struct tmpfs_args *, struct proc *);
void	tmpfs_mountfs_getparams(const struct tmpfs_args *, uint64_t *,
	    uint64_t *);

int
tmpfs_init(struct vfsconf *vfsp)
{

	pool_init(&tmpfs_dirent_pool, sizeof(tmpfs_dirent_t), 0, 0, 0,
	    "tmpfs_dirent", &pool_allocator_nointr);
	pool_init(&tmpfs_node_pool, sizeof(tmpfs_node_t), 0, 0, 0,
	    "tmpfs_node", &pool_allocator_nointr);

	return 0;
}

void
tmpfs_mountfs_getparams(const struct tmpfs_args *args, uint64_t *memlimit,
    uint64_t *nodes)
{
	/* Get the memory usage limit for this file-system. */
	if (args->ta_size_max < PAGE_SIZE) {
		*memlimit = UINT64_MAX;
	} else {
		*memlimit = args->ta_size_max;
	}
	KASSERT(*memlimit > 0);

	if (args->ta_nodes_max <= 3) {
		*nodes = 3 + (*memlimit / 1024);
	} else {
		*nodes = args->ta_nodes_max;
	}
	*nodes = MIN(*nodes, INT_MAX);
	KASSERT(*nodes >= 3);
}

int
tmpfs_mount_update(struct mount *mp, struct tmpfs_args *args, struct proc *p)
{
	tmpfs_mount_t *tmp;
	struct vnode *rootvp;
	char fspec[MAXPATHLEN];
	int error = 0;

	tmp = mp->mnt_data;
	rootvp = tmp->tm_root->tn_vnode;

	/* Lock root to prevent lookups. */
	error = vn_lock(rootvp, LK_EXCLUSIVE | LK_RETRY, curproc);
	if (error)
		return error;
	
	/* Lock mount point to prevent nodes from being added/removed. */
	rw_enter_write(&tmp->tm_lock);

	if (mp->mnt_flag & MNT_SNAPSHOT) {
		error = copyinstr(args->ta_fspec, fspec, sizeof(fspec), NULL);
		if (error)
			goto bail;

		error = tmpfs_snap_dump(mp, fspec, p);
		if (error)
			goto bail;
	}

	/* ro->rw transition: nothing to do? */
	if (mp->mnt_flag & MNT_WANTRDWR) {
		error = 0;
		goto bail;
	} else if (mp->mnt_flag & MNT_RDONLY) {
		/* Flush files opened for writing; skip rootvp. */
		error = vflush(mp, rootvp, WRITECLOSE);
		if (error)
			goto bail;
	}

	if (args->ta_nodes_max != 0 || args->ta_size_max != 0) {
		struct tmpfs_args *xargs = &mp->mnt_stat.mount_info.tmpfs_args;
		uint64_t memlimit, nodelimit;

		tmpfs_mountfs_getparams(args, &memlimit, &nodelimit);

		if (args->ta_nodes_max != 0) {
			if (nodelimit < tmp->tm_nodes_cnt) {
				error = EINVAL;
				goto bail;
			}
			tmp->tm_nodes_max = (unsigned int)nodelimit;
			xargs->ta_nodes_max = nodelimit;
		}

		if (args->ta_size_max != 0) {
			error = tmpfs_mntmem_adjust(tmp, memlimit);
			if (error)
				goto bail;
			xargs->ta_size_max = memlimit;
		}

	}

bail:
	rw_exit_write(&tmp->tm_lock);
	VOP_UNLOCK(rootvp, 0);

	return error;
}

int
tmpfs_mountfs(struct mount *mp, const char *path, struct vnode *vp,
    struct tmpfs_args *args, char *fspec, struct proc *p)
{
	tmpfs_mount_t *tmp;
	tmpfs_node_t *root;
	uint64_t memlimit;
	uint64_t nodes;
	int error;

	if (mp->mnt_flag & MNT_UPDATE)
		return (tmpfs_mount_update(mp, args, p));

	if (mp->mnt_flag & MNT_SNAPSHOT)
		return EINVAL;

	/* Prohibit mounts if there is not enough memory. */
	if (tmpfs_mem_info(1) < TMPFS_PAGES_RESERVED)
		return EINVAL;

	/* Allocate the tmpfs mount structure and fill it. */
	tmp = malloc(sizeof(tmpfs_mount_t), M_MISCFSMNT, M_WAITOK);
	if (tmp == NULL)
		return ENOMEM;

	tmpfs_mountfs_getparams(args, &memlimit, &nodes);

	tmp->tm_nodes_max = (unsigned int)nodes;
	tmp->tm_nodes_cnt = 0;
	tmp->tm_highest_inode = 1;
	strlcpy(tmp->tm_fspec, fspec, sizeof(tmp->tm_fspec));
	LIST_INIT(&tmp->tm_nodes);

	rw_init(&tmp->tm_lock, "tmplk");
	tmpfs_mntmem_init(tmp, memlimit);

	/* Allocate the root node. */
	error = tmpfs_alloc_node(tmp, VDIR, args->ta_root_uid,
	    args->ta_root_gid, args->ta_root_mode & ALLPERMS, NULL,
	    VNOVAL, &root);
	KASSERT(error == 0 && root != NULL);

	/*
	 * Parent of the root inode is itself.  Also, root inode has no
	 * directory entry (i.e. is never attached), thus hold an extra
	 * reference (link) for it.
	 */
	root->tn_links++;
	root->tn_spec.tn_dir.tn_parent = root;
	tmp->tm_root = root;

	mp->mnt_data = tmp;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_stat.f_namemax = TMPFS_MAXNAMLEN;
	vfs_getnewfsid(mp);

	mp->mnt_stat.mount_info.tmpfs_args = *args;

	bzero(&mp->mnt_stat.f_mntonname, sizeof(mp->mnt_stat.f_mntonname));
	bzero(&mp->mnt_stat.f_mntfromname, sizeof(mp->mnt_stat.f_mntfromname));
	bzero(&mp->mnt_stat.f_mntfromspec, sizeof(mp->mnt_stat.f_mntfromspec));

	strlcpy(mp->mnt_stat.f_mntonname, path,
	    sizeof(mp->mnt_stat.f_mntonname) - 1);
	strlcpy(mp->mnt_stat.f_mntfromname, tmp->tm_fspec,
	    sizeof(mp->mnt_stat.f_mntfromname) - 1);
	strlcpy(mp->mnt_stat.f_mntfromspec, tmp->tm_fspec,
	    sizeof(mp->mnt_stat.f_mntfromspec) - 1);

	if (vp != NULL) {
		error = VOP_OPEN(vp, FREAD, p->p_ucred);
		if (error)
			goto bail;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		error = tmpfs_snap_load_vnode(mp, vp, p);
		VOP_CLOSE(vp, FREAD, p->p_ucred);
		VOP_UNLOCK(vp, 0);
	} else if (strcmp("tmpfs", fspec))
		error = tmpfs_snap_load(mp, fspec, p);

bail:
	if (error)
		free(tmp, M_MISCFSMNT);

	return (error);
}

#define TMPFSROOT_NAME	"/dev/tmpfsrd0a"

int
tmpfs_mountroot(void)
{
	struct mount *mp;
	struct tmpfs_mount *tmp;
	struct proc *p = curproc;       /* XXX */
	struct tmpfs_args args;
	int error;

	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	swapdev_vp = NULL;
	if ((error = bdevvp(swapdev, &swapdev_vp)) ||
	    (error = bdevvp(rootdev, &rootvp))) {
		printf("tmpfs_mountroot: can't setup bdevvp's\n");
		if (swapdev_vp)
			vrele(swapdev_vp);
		return (error);
	}

	if ((error = vfs_rootmountalloc("tmpfs", TMPFSROOT_NAME, &mp)) != 0) {
		vrele(swapdev_vp);
		vrele(rootvp);
		return (error);
	}

	bzero(&args, sizeof(args));
	error = tmpfs_mountfs(mp, "/", rootvp, &args, TMPFSROOT_NAME, p);
	if (error) {
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vrele(swapdev_vp);
		vrele(rootvp);
		return (error);
	}

	TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);

	tmp = VFS_TO_TMPFS(mp);
	vfs_unbusy(mp);
	inittodr(tmp->tm_root->tn_ctime.tv_sec);

	return (0);
}

int
tmpfs_mount(struct mount *mp, const char *path, void *data,
    struct nameidata *ndp, struct proc *p)
{
	struct tmpfs_args args;
	char fspec[MAXPATHLEN];
	int error;

	error = copyin(data, &args, sizeof(struct tmpfs_args));
	if (error)
		return (error);

	error = copyinstr(args.ta_fspec, fspec, sizeof(fspec), NULL);
	if (error)
		return error;

	return (tmpfs_mountfs(mp, path, NULL, &args, fspec, p));
}

int
tmpfs_start(struct mount *mp, int flags, struct proc *p)
{

	return 0;
}

int
tmpfs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
	tmpfs_node_t *node, *cnode;
	int error, flags = 0;

	/* Handle forced unmounts. */
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	/* Finalize all pending I/O. */
	error = vflush(mp, NULL, flags);
	if (error != 0)
		return error;

	/*
	 * First round, detach and destroy all directory entries.
	 * Also, clear the pointers to the vnodes - they are gone.
	 */
	LIST_FOREACH(node, &tmp->tm_nodes, tn_entries) {
		tmpfs_dirent_t *de;

		node->tn_vnode = NULL;
		if (node->tn_type != VDIR) {
			continue;
		}
		while ((de = TAILQ_FIRST(&node->tn_spec.tn_dir.tn_dir)) != NULL) {
			cnode = de->td_node;
			if (cnode)
				cnode->tn_vnode = NULL;
			tmpfs_dir_detach(node, de);
			tmpfs_free_dirent(tmp, de);
		}
	}

	/* Second round, destroy all inodes. */
	while ((node = LIST_FIRST(&tmp->tm_nodes)) != NULL) {
		tmpfs_free_node(tmp, node);
	}

	/* Throw away the tmpfs_mount structure. */
	tmpfs_mntmem_destroy(tmp);
	/* mutex_destroy(&tmp->tm_lock); */
	/* kmem_free(tmp, sizeof(*tmp)); */
	free(tmp, M_MISCFSMNT);
	mp->mnt_data = NULL;

	return 0;
}

int
tmpfs_root(struct mount *mp, struct vnode **vpp)
{
	tmpfs_node_t *node = VFS_TO_TMPFS(mp)->tm_root;

	rw_enter_write(&node->tn_nlock);
	return tmpfs_vnode_get(mp, node, vpp);
}

int
tmpfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{

	printf("tmpfs_vget called; need for it unknown yet\n");
	return EOPNOTSUPP;
}

int
tmpfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
	tmpfs_node_t *node;
	tmpfs_fid_t tfh;

	if (fhp->fid_len != sizeof(tmpfs_fid_t)) {
		return EINVAL;
	}
	memcpy(&tfh, fhp, sizeof(tmpfs_fid_t));

	rw_enter_write(&tmp->tm_lock);
	LIST_FOREACH(node, &tmp->tm_nodes, tn_entries) {
		if (node->tn_id != tfh.tf_id) {
			continue;
		}
		if (TMPFS_NODE_GEN(node) != tfh.tf_gen) {
			continue;
		}
		rw_enter_write(&node->tn_nlock);
		break;
	}
	rw_exit_write(&tmp->tm_lock);

	/* Will release the tn_nlock. */
	return node ? tmpfs_vnode_get(mp, node, vpp) : ESTALE;
}

int
tmpfs_vptofh(struct vnode *vp, struct fid *fhp)
{
	tmpfs_fid_t tfh;
	tmpfs_node_t *node;

	node = VP_TO_TMPFS_NODE(vp);

	memset(&tfh, 0, sizeof(tfh));
	tfh.tf_len = sizeof(tmpfs_fid_t);
	tfh.tf_gen = (uint32_t)TMPFS_NODE_GEN(node);
	tfh.tf_id = node->tn_id;
	memcpy(fhp, &tfh, sizeof(tfh));

	return 0;
}

int
tmpfs_statfs(struct mount *mp, struct statfs *sbp, struct proc *p)
{
	tmpfs_mount_t *tmp;
	fsfilcnt_t freenodes;
	uint64_t avail;

	tmp = VFS_TO_TMPFS(mp);

	sbp->f_iosize = sbp->f_bsize = PAGE_SIZE;

	mtx_enter(&tmp->tm_acc_lock);
	avail =  tmpfs_pages_avail(tmp);
	sbp->f_blocks = (tmpfs_bytes_max(tmp) >> PAGE_SHIFT);
	sbp->f_bfree = avail;
	sbp->f_bavail = avail & INT64_MAX; /* f_bavail is int64_t */

	freenodes = MIN(tmp->tm_nodes_max - tmp->tm_nodes_cnt,
	    avail * PAGE_SIZE / sizeof(tmpfs_node_t));

	sbp->f_files = tmp->tm_nodes_cnt + freenodes;
	sbp->f_ffree = freenodes;
	sbp->f_favail = freenodes & INT64_MAX; /* f_favail is int64_t */
	mtx_leave(&tmp->tm_acc_lock);

	sbp->f_fsid = mp->mnt_stat.f_fsid;
	sbp->f_owner = mp->mnt_stat.f_owner;
	sbp->f_flags = mp->mnt_stat.f_flags;
	sbp->f_syncwrites = mp->mnt_stat.f_syncwrites;
	sbp->f_asyncwrites = mp->mnt_stat.f_asyncwrites;
	sbp->f_syncreads = mp->mnt_stat.f_syncreads;
	sbp->f_asyncreads = mp->mnt_stat.f_asyncreads;
	sbp->f_namemax = mp->mnt_stat.f_namemax;

	strncpy(sbp->f_fstypename, mp->mnt_vfc->vfc_name, MFSNAMELEN);
	bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
	bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	bcopy(mp->mnt_stat.f_mntfromspec, sbp->f_mntfromspec, MNAMELEN);
	bcopy(&mp->mnt_stat.mount_info.tmpfs_args, &sbp->mount_info.tmpfs_args,
	    sizeof(struct tmpfs_args));

	return 0;
}

int
tmpfs_sync(struct mount *mp, int waitfor, struct ucred *cred, struct proc *p)
{

	return 0;
}

/*
 * tmpfs vfs operations.
 */

struct vfsops tmpfs_vfsops = {
	tmpfs_mount,			/* vfs_mount */
	tmpfs_start,			/* vfs_start */
	tmpfs_unmount,			/* vfs_unmount */
	tmpfs_root,			/* vfs_root */
	NULL,				/* vfs_quotactl */
	tmpfs_statfs,			/* vfs_statfs */
	tmpfs_sync,			/* vfs_sync */
	tmpfs_vget,			/* vfs_vget */
	tmpfs_fhtovp,			/* vfs_fhtovp */
	tmpfs_vptofh,			/* vfs_vptofh */
	tmpfs_init,			/* vfs_init */
	NULL,				/* vfs_sysctl */
	(void *)eopnotsupp,		/* vfs_checkexp */
};
