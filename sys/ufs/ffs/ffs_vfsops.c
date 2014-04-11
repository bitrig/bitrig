/*	$OpenBSD: ffs_vfsops.c,v 1.140 2013/12/12 19:00:10 tedu Exp $	*/
/*	$NetBSD: ffs_vfsops.c,v 1.19 1996/02/09 22:22:26 christos Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc, and by Andrew Doran.
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
 * Copyright (c) 1989, 1991, 1993, 1994
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
 *
 *	@(#)ffs_vfsops.c	8.14 (Berkeley) 11/28/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/pool.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/wapbl.h>

#include <dev/rndvar.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ufs/dirhash.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

int ffs_sbupdate(struct ufsmount *, int);
int ffs_reload_vnode(struct vnode *, void *);
int ffs_sync_vnode(struct vnode *, void *);
int ffs_validate(struct fs *);

void ffs1_compat_read(struct fs *, struct ufsmount *, daddr_t);
void ffs1_compat_write(struct fs *, struct ufsmount *);

const struct vfsops ffs_vfsops = {
	ffs_mount,
	ufs_start,
	ffs_unmount,
	ufs_root,
	ufs_quotactl,
	ffs_statfs,
	ffs_sync,
	ffs_vget,
	ffs_fhtovp,
	ffs_vptofh,
	ffs_init,
	ffs_sysctl,
	ufs_check_export
};

struct inode_vtbl ffs_vtbl = {
	ffs_truncate,
	ffs_update,
	ffs_inode_alloc,
	ffs_inode_free,
	ffs_balloc,
	ffs_bufatoff
};


/*
 * Called by main() when ufs is going to be mounted as root.
 */

struct pool ffs_ino_pool;
struct pool ffs_dinode1_pool;
#ifdef FFS2
struct pool ffs_dinode2_pool;
#endif

int
ffs_mountroot(void)
{
	struct fs *fs;
	struct mount *mp;
	struct proc *p = curproc;	/* XXX */
	struct ufsmount *ump;
	int error;

	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	swapdev_vp = NULL;
	if ((error = bdevvp(swapdev, &swapdev_vp)) ||
	    (error = bdevvp(rootdev, &rootvp))) {
		printf("ffs_mountroot: can't setup bdevvp's\n");
		if (swapdev_vp)
			vrele(swapdev_vp);
		return (error);
	}

	if ((error = vfs_rootmountalloc("ffs", "root_device", &mp)) != 0) {
		vrele(swapdev_vp);
		vrele(rootvp);
		return (error);
	}

	if ((error = ffs_mountfs(rootvp, mp, p)) != 0) {
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vrele(swapdev_vp);
		vrele(rootvp);
		return (error);
	}

	TAILQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	strlcpy(fs->fs_fsmnt, mp->mnt_stat.f_mntonname, sizeof(fs->fs_fsmnt));
	(void)ffs_statfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp);
	inittodr(fs->fs_time);

	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
ffs_mount(struct mount *mp, const char *path, void *data,
    struct nameidata *ndp, struct proc *p)
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump = NULL;
	struct fs *fs;
	char fname[MNAMELEN];
	char fspec[MNAMELEN];
	int error = 0, flags;
	int ronly;
	mode_t accessmode;

	error = copyin(data, &args, sizeof(struct ufs_args));
	if (error)
		return (error);

#ifdef WAPBL
	/* WAPBL can only be enabled on a r/w mount. */
	if ((mp->mnt_flag & MNT_RDONLY) && !(mp->mnt_flag & MNT_WANTRDWR)) {
		mp->mnt_flag &= ~MNT_LOG;
	}
#else /* !WAPBL */
	mp->mnt_flag &= ~MNT_LOG;
#endif /* !WAPBL */

#ifndef FFS_SOFTUPDATES
	if (mp->mnt_flag & MNT_SOFTDEP) {
		printf("WARNING: soft updates isn't compiled in\n");
		mp->mnt_flag &= ~MNT_SOFTDEP;
	}
#endif

	/*
	 * Soft updates is incompatible with "async",
	 * so if we are doing softupdates stop the user
	 * from setting the async flag.
	 */
	if ((mp->mnt_flag & (MNT_SOFTDEP | MNT_ASYNC)) ==
	    (MNT_SOFTDEP | MNT_ASYNC)) {
		return (EINVAL);
	}

#ifdef WAPBL
	/*
	 * Likewise, WAPBL is incompatible with MNT_ASYNC and MNT_SOFTDEP.
	 */
	if (mp->mnt_flag & MNT_LOG)
		if (mp->mnt_flag & (MNT_ASYNC|MNT_SOFTDEP))
			return (EINVAL);
#endif

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		devvp = ump->um_devvp;
		error = 0;
		ronly = fs->fs_ronly;

#ifdef WAPBL
		/* XXX pedro */
		if (mp->mnt_wapbl != NULL && (mp->mnt_flag & MNT_LOG) == 0)
			return (EPERM);
#endif

		if (ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			/* Flush any dirty data */
			mp->mnt_flag &= ~MNT_RDONLY;
			VFS_SYNC(mp, MNT_WAIT, p->p_ucred, p);
			mp->mnt_flag |= MNT_RDONLY;

			/*
			 * Get rid of files open for writing.
			 */
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if (fs->fs_flags & FS_DOSOFTDEP) {
				error = softdep_flushfiles(mp, flags, p);
				mp->mnt_flag &= ~MNT_SOFTDEP;
			} else {
				error = ffs_flushfiles(mp, flags, p);
				if (error == 0)
					error = UFS_WAPBL_BEGIN(mp);
				if (error == 0 &&
				    ffs_cgupdate(ump, MNT_WAIT) == 0 &&
				    fs->fs_clean & FS_WASCLEAN) {
				    	fs->fs_clean = FS_ISCLEAN;
				    	(void) ffs_sbupdate(ump, MNT_WAIT);
				}
				if (error == 0)
					UFS_WAPBL_END(mp);
			}
			ronly = 1;
		}

#ifdef WAPBL
		if (error == 0 && (mp->mnt_flag & MNT_LOG) == 0) {
			error = ffs_wapbl_stop(mp, mp->mnt_flag & MNT_FORCE);
			if (error)
				return (error);
		}
#endif /* WAPBL */

		/*
		 * Flush soft dependencies if disabling it via an update
		 * mount. This may leave some items to be processed,
		 * so don't do this yet XXX.
		 */
		if ((fs->fs_flags & FS_DOSOFTDEP) &&
		    !(mp->mnt_flag & MNT_SOFTDEP) &&
		    !(mp->mnt_flag & MNT_RDONLY) && fs->fs_ronly == 0) {
#if 0
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = softdep_flushfiles(mp, flags, p);
#elif FFS_SOFTUPDATES
			mp->mnt_flag |= MNT_SOFTDEP;
#endif
		}
		/*
		 * When upgrading to a softdep mount, we must first flush
		 * all vnodes. (not done yet -- see above)
		 */
		if (!(fs->fs_flags & FS_DOSOFTDEP) &&
		    (mp->mnt_flag & MNT_SOFTDEP) && fs->fs_ronly == 0) {
#if 0
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = ffs_flushfiles(mp, flags, p);
#else
			mp->mnt_flag &= ~MNT_SOFTDEP;
#endif
		}

		if (!error && (mp->mnt_flag & MNT_RELOAD))
			error = ffs_reload(mp, ndp->ni_cnd.cn_cred, p);
		if (error)
			goto error_1;

		if (ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			if (suser(p, 0)) {
				vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
				error = VOP_ACCESS(devvp, VREAD | VWRITE,
						   p->p_ucred);
				VOP_UNLOCK(devvp, 0);
				if (error)
					goto error_1;
			}

			fs->fs_contigdirs = malloc((u_long)fs->fs_ncg,
			     M_UFSMNT, M_WAITOK|M_ZERO);

#ifdef WAPBL
			if (mp->mnt_flag & MNT_LOG) {
				fs->fs_ronly = 0;
				fs->fs_fmod = 1;
			}

			if (fs->fs_flags & FS_DOWAPBL) {
				printf("%s: replaying log to disk\n",
				    mp->mnt_stat.f_mntonname);
				KASSERT(mp->mnt_wapbl_replay);
				error = wapbl_replay_write(mp->mnt_wapbl_replay,
				    devvp);
				if (error) {
					free(fs->fs_contigdirs, M_UFSMNT);
					return (error);
				}
				wapbl_replay_stop(mp->mnt_wapbl_replay);
				fs->fs_clean = FS_WASCLEAN;
				goto logok;
			}
#endif /* WAPBL */

			if (fs->fs_clean == 0) {
#if 0
				/*
				 * It is safe to mount an unclean file system
				 * if it was previously mounted with softdep
				 * but we may lose space and must
				 * sometimes run fsck manually.
				 */
				if (fs->fs_flags & FS_DOSOFTDEP)
					printf(
"WARNING: %s was not properly unmounted\n",
					    fs->fs_fsmnt);
				else
#endif
				if (mp->mnt_flag & MNT_FORCE) {
					printf(
"WARNING: %s was not properly unmounted\n",
					    fs->fs_fsmnt);
				} else {
					printf(
"WARNING: R/W mount of %s denied.  Filesystem is not clean - run fsck\n",
					    fs->fs_fsmnt);
					error = EROFS;
					free(fs->fs_contigdirs, M_UFSMNT);
					goto error_1;
				}
			}

			if ((fs->fs_flags & FS_DOSOFTDEP)) {
				error = softdep_mount(devvp, mp, fs,
						      p->p_ucred);
				if (error) {
					free(fs->fs_contigdirs, M_UFSMNT);
					goto error_1;
				}
			}

#ifdef WAPBL
logok:
#endif
			ronly = 0;
		}

#ifdef WAPBL
		error = ffs_wapbl_start(mp);
		if (error)
			return error;
#endif /* WAPBL */

		if (args.fspec == NULL) {
			/*
			 * Process export requests.
			 */
			error = vfs_export(mp, &ump->um_export, 
			    &args.export_info);
			if (error)
				goto error_1;
			else {
				error = UFS_WAPBL_BEGIN(mp);
				if (error)
					goto error_1;
				goto success;
			}
		}
	}

	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	error = copyinstr(args.fspec, fspec, sizeof(fspec), NULL);
	if (error)
		goto error_1;

	if (disk_map(fspec, fname, MNAMELEN, DM_OPENBLCK) == -1)
		memcpy(fname, fspec, sizeof(fname));

	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, fname, p);
	if ((error = namei(ndp)) != 0)
		goto error_1;

	devvp = ndp->ni_vp;

	if (devvp->v_type != VBLK) {
		error = ENOTBLK;
		goto error_2;
	}

	if (major(devvp->v_rdev) >= nblkdev) {
		error = ENXIO;
		goto error_2;
	}

	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (suser(p, 0)) {
		accessmode = VREAD;
		if ((mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
		error = VOP_ACCESS(devvp, accessmode, p->p_ucred);
		VOP_UNLOCK(devvp, 0);
		if (error)
			goto error_2;
	}

	if (mp->mnt_flag & MNT_UPDATE) {
		/*
		 * UPDATE
		 * If it's not the same vnode, or at least the same device
		 * then it's not correct.
		 */

		if (devvp != ump->um_devvp) {
			if (devvp->v_rdev == ump->um_devvp->v_rdev) {
				vrele(devvp);
			} else {
				error = EINVAL;	/* needs translation */
			}
		} else
			vrele(devvp);
		/*
		 * Update device name only on success
		 */
		if (!error) {
			/*
			 * Save "mounted from" info for mount point (NULL pad)
			 */
			memset(mp->mnt_stat.f_mntfromname, 0, MNAMELEN);
			strlcpy(mp->mnt_stat.f_mntfromname, fname, MNAMELEN);
			memset(mp->mnt_stat.f_mntfromspec, 0, MNAMELEN);
			strlcpy(mp->mnt_stat.f_mntfromspec, fspec, MNAMELEN);
		}
	} else {
		/*
		 * Since this is a new mount, we want the names for
		 * the device and the mount point copied in.  If an
		 * error occurs,  the mountpoint is discarded by the
		 * upper level code.
		 */
		memset(mp->mnt_stat.f_mntonname, 0, MNAMELEN);
		strlcpy(mp->mnt_stat.f_mntonname, path, MNAMELEN);
		memset(mp->mnt_stat.f_mntfromname, 0, MNAMELEN);
		strlcpy(mp->mnt_stat.f_mntfromname, fname, MNAMELEN);
		memset(mp->mnt_stat.f_mntfromspec, 0, MNAMELEN);
		strlcpy(mp->mnt_stat.f_mntfromspec, fspec, MNAMELEN);

		error = ffs_mountfs(devvp, mp, p);
	}

	if (error)
		goto error_2;

	error = UFS_WAPBL_BEGIN(mp);
	if (error)
		goto error_2;

	/*
	 * Initialize FS stat information in mount struct; uses both
	 * mp->mnt_stat.f_mntonname and mp->mnt_stat.f_mntfromname
	 *
	 * This code is common to root and non-root mounts
	 */
	memcpy(&mp->mnt_stat.mount_info.ufs_args, &args, sizeof(args));
	VFS_STATFS(mp, &mp->mnt_stat, p);

success:
	if (path && (mp->mnt_flag & MNT_UPDATE)) {
		/* Update clean flag after changing read-onlyness. */
		fs = ump->um_fs;
		/*
		 * XXX pedro: should use FS_ISCLEAN below when setting fs_clean
		 * for consistency between the kernel and userland
		 */
		if (ronly != fs->fs_ronly) {
			fs->fs_ronly = ronly;
			fs->fs_clean = ronly &&
			    (fs->fs_flags & FS_UNCLEAN) == 0 ? 1 : 0;
			if (ronly)
				free(fs->fs_contigdirs, M_UFSMNT);
		}
		if (!ronly) {
			if (mp->mnt_flag & MNT_SOFTDEP)
				fs->fs_flags |= FS_DOSOFTDEP;
			else
				fs->fs_flags &= ~FS_DOSOFTDEP;
		}
		ffs_sbupdate(ump, MNT_WAIT);
	}
	UFS_WAPBL_END(mp);
	return (0);

error_2:	/* error with devvp held */
	vrele (devvp);

error_1:	/* no state to back out */
	return (error);
}

struct ffs_reload_args {
	struct fs *fs;
	struct proc *p;
	struct ucred *cred;
	struct vnode *devvp;
};

int
ffs_reload_vnode(struct vnode *vp, void *args) 
{
	struct ffs_reload_args *fra = args;
	struct inode *ip;
	struct buf *bp;
	int error;

	/*
	 * Step 4: invalidate all inactive vnodes.
	 */
	if (vp->v_usecount == 0) {
		vgonel(vp, fra->p);
		return (0);
	}

	/*
	 * Step 5: invalidate all cached file data.
	 */
	if (vget(vp, LK_EXCLUSIVE, fra->p))
		return (0);

	if (vinvalbuf(vp, 0, fra->cred, fra->p, 0, 0))
		panic("ffs_reload: dirty2");

	/*
	 * Step 6: re-read inode data for all active vnodes.
	 */
	ip = VTOI(vp);

	error = bread(fra->devvp, 
	    fsbtodb(fra->fs, ino_to_fsba(fra->fs, ip->i_number)),
	    (int)fra->fs->fs_bsize, &bp);
	if (error) {
		brelse(bp);
		vput(vp);
		return (error);
	}

	*ip->i_din1 = *((struct ufs1_dinode *)bp->b_data +
	    ino_to_fsbo(fra->fs, ip->i_number));
	ip->i_effnlink = DIP(ip, nlink);
	brelse(bp);
	vput(vp);
	return (0);
}

/*
 * Reload all incore data for a filesystem (used after running fsck on
 * the root filesystem and finding things to fix). The filesystem must
 * be mounted read-only.
 *
 * Things to do to update the mount:
 *	1) invalidate all cached meta-data.
 *	2) re-read superblock from disk.
 *	3) re-read summary information from disk.
 *	4) invalidate all inactive vnodes.
 *	5) invalidate all cached file data.
 *	6) re-read inode data for all active vnodes.
 */
int
ffs_reload(struct mount *mountp, struct ucred *cred, struct proc *p)
{
	struct vnode *devvp;
	caddr_t space;
	struct fs *fs, *newfs;
	int i, blks, size, error;
	int32_t *lp;
	struct buf *bp = NULL;
	struct ffs_reload_args fra;

	if ((mountp->mnt_flag & MNT_RDONLY) == 0)
		return (EINVAL);
	/*
	 * Step 1: invalidate all cached meta-data.
	 */
	devvp = VFSTOUFS(mountp)->um_devvp;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
	error = vinvalbuf(devvp, 0, cred, p, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		panic("ffs_reload: dirty1");

	/*
	 * Step 2: re-read superblock from disk.
	 */
	fs = VFSTOUFS(mountp)->um_fs;

	error = bread(devvp, fs->fs_sblockloc / DEV_BSIZE, SBSIZE, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}

	newfs = (struct fs *)bp->b_data;
	if (ffs_validate(newfs) == 0) {
		brelse(bp);
		return (EINVAL);
	}

	/*
	 * Copy pointer fields back into superblock before copying in	XXX
	 * new superblock. These should really be in the ufsmount.	XXX
	 * Note that important parameters (eg fs_ncg) are unchanged.
	 */
	newfs->fs_csp = fs->fs_csp;
	newfs->fs_maxcluster = fs->fs_maxcluster;
	newfs->fs_ronly = fs->fs_ronly;
	memcpy(fs, newfs, fs->fs_sbsize);
	if (fs->fs_sbsize < SBSIZE)
		bp->b_flags |= B_INVAL;
	brelse(bp);
	mountp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	ffs1_compat_read(fs, VFSTOUFS(mountp), fs->fs_sblockloc);
	ffs_oldfscompat(fs);
	(void)ffs_statfs(mountp, &mountp->mnt_stat, p);
	/*
	 * Step 3: re-read summary information from disk.
	 */
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + i), size, &bp);
		if (error) {
			brelse(bp);
			return (error);
		}
		memcpy(space, bp->b_data, size);
		space += size;
		brelse(bp);
	}
	if ((fs->fs_flags & FS_DOSOFTDEP))
		(void) softdep_mount(devvp, mountp, fs, cred);
	/*
	 * We no longer know anything about clusters per cylinder group.
	 */
	if (fs->fs_contigsumsize > 0) {
		lp = fs->fs_maxcluster;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}

	fra.p = p;
	fra.cred = cred;
	fra.fs = fs;
	fra.devvp = devvp;

	error = vfs_mount_foreach_vnode(mountp, ffs_reload_vnode, &fra);

	return (error);
}

/*
 * Checks if a super block is sane enough to be mounted.
 */
int
ffs_validate(struct fs *fsp)
{
#ifdef FFS2
	if (fsp->fs_magic != FS_UFS2_MAGIC && fsp->fs_magic != FS_UFS1_MAGIC)
		return (0); /* Invalid magic */
#else
	if (fsp->fs_magic != FS_UFS1_MAGIC)
		return (0); /* Invalid magic */
#endif /* FFS2 */

	if ((u_int)fsp->fs_bsize > MAXBSIZE)
		return (0); /* Invalid block size */

	if ((u_int)fsp->fs_bsize < sizeof(struct fs))
		return (0); /* Invalid block size */

	if ((u_int)fsp->fs_sbsize > SBSIZE)
		return (0); /* Invalid super block size */

	if ((u_int)fsp->fs_frag > MAXFRAG || fragtbl[fsp->fs_frag] == NULL)
		return (0); /* Invalid number of fragments */

	return (1); /* Super block is okay */
}

/*
 * Possible locations for the super-block.
 */
const int sbtry[] = SBLOCKSEARCH;

/*
 * Common code for mount and mountroot
 */
int
ffs_mountfs(struct vnode *devvp, struct mount *mp, struct proc *p)
{
	struct ufsmount *ump;
	struct buf *bp;
	struct fs *fs;
	dev_t dev;
	caddr_t space;
	daddr_t sbloc;
	int error, i, blks, size, ronly;
	int32_t *lp;
	struct ucred *cred;
	u_int64_t maxfilesize;					/* XXX */

	dev = devvp->v_rdev;
	cred = p ? p->p_ucred : NOCRED;
	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if ((error = vfs_mountedon(devvp)) != 0)
		return (error);
	if (vcount(devvp) > 1 && devvp != rootvp)
		return (EBUSY);
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
	error = vinvalbuf(devvp, V_SAVE, cred, p, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED);
	if (error)
		return (error);

	bp = NULL;
	ump = NULL;

#ifdef WAPBL
sbagain:
#endif
	/*
	 * Try reading the super-block in each of its possible locations.
	 */
	for (i = 0; sbtry[i] != -1; i++) {
		if (bp != NULL) {
			bp->b_flags |= B_NOCACHE;
			brelse(bp);
			bp = NULL;
		}

		error = bread(devvp, sbtry[i] / DEV_BSIZE, SBSIZE, &bp);
		if (error)
			goto out;

		fs = (struct fs *) bp->b_data;
		sbloc = sbtry[i];

#if 0
		if (fs->fs_magic == FS_UFS2_MAGIC) {
			printf("ffs_mountfs(): Sorry, no UFS2 support (yet)\n");
			error = EFTYPE;
			goto out;
		}
#endif

		/*
		 * Do not look for an FFS1 file system at SBLOCK_UFS2. Doing so
		 * will find the wrong super-block for file systems with 64k
		 * block size.
		 */
		if (fs->fs_magic == FS_UFS1_MAGIC && sbloc == SBLOCK_UFS2)
			continue;

		if (ffs_validate(fs))
			break; /* Super block validated */
	}

	if (sbtry[i] == -1) {
		error = EINVAL;
		goto out;
	}

#ifdef WAPBL
	if ((mp->mnt_wapbl_replay == 0) && (fs->fs_flags & FS_DOWAPBL)) {
		error = ffs_wapbl_replay_start(mp, fs, devvp);
		if (error && (mp->mnt_flag & MNT_FORCE) == 0)
			goto out;
		if (!error) {
			if (!ronly) {
				/* XXX fsmnt may be stale. */
				printf("%s: replaying log to disk\n",
				    fs->fs_fsmnt);
				error = wapbl_replay_write(mp->mnt_wapbl_replay,
				    devvp);
				if (error)
					goto out;
				wapbl_replay_stop(mp->mnt_wapbl_replay);
				fs->fs_clean = FS_WASCLEAN;
			} else {
				/* XXX fsmnt may be stale */
				printf("%s: replaying log to memory\n",
				    fs->fs_fsmnt);
			}

			/* Force a re-read of the superblock */
			bp->b_flags |= B_INVAL;
			brelse(bp);
			bp = NULL;
			fs = NULL;
			goto sbagain;
		}
	}
#else /* !WAPBL */
	if ((fs->fs_flags & FS_DOWAPBL) && (mp->mnt_flag & MNT_FORCE) == 0) {
		error = EPERM;
		goto out;
	}
#endif /* !WAPBL */

	fs->fs_fmod = 0;
	fs->fs_flags &= ~FS_UNCLEAN;

	/* XXX pedro */
	if (fs->fs_clean == 0 && mp->mnt_wapbl_replay)
		fs->fs_clean = FS_ISCLEAN;

	if (fs->fs_clean == 0) {
#if 0
		/*
		 * It is safe to mount an unclean file system
		 * if it was previously mounted with softdep
		 * but we may lose space and must
		 * sometimes run fsck manually.
		 */
		if (fs->fs_flags & FS_DOSOFTDEP)
			printf(
"WARNING: %s was not properly unmounted\n",
			    fs->fs_fsmnt);
		else
#endif
		if (ronly || (mp->mnt_flag & MNT_FORCE)) {
			printf(
"WARNING: %s was not properly unmounted\n",
			    fs->fs_fsmnt);
		} else {
			printf(
"WARNING: R/W mount of %s denied.  Filesystem is not clean - run fsck\n",
			    fs->fs_fsmnt);
			error = EROFS;
			goto out;
		}
	}

	if (fs->fs_postblformat == FS_42POSTBLFMT && !ronly) {
#ifndef SMALL_KERNEL
		printf("ffs_mountfs(): obsolete rotational table format, "
		    "please use fsck_ffs(8) -c 1\n");
#endif
		error = EFTYPE;
		goto out;
	}

	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK|M_ZERO);
	ump->um_fs = malloc((u_long)fs->fs_sbsize, M_UFSMNT,
	    M_WAITOK);

	if (fs->fs_magic == FS_UFS1_MAGIC)
		ump->um_fstype = UM_UFS1;
#ifdef FFS2
	else
		ump->um_fstype = UM_UFS2;
#endif

	memcpy(ump->um_fs, bp->b_data, fs->fs_sbsize);
	if (fs->fs_sbsize < SBSIZE)
		bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;
	fs = ump->um_fs;

	ffs1_compat_read(fs, ump, sbloc);

	/* Don't bump fs_clean if we're replaying journal */
	if (!((fs->fs_flags & FS_DOWAPBL) && (fs->fs_clean & FS_WASCLEAN)))
		if (ronly == 0) {
			fs->fs_clean <<= 1;
			fs->fs_fmod = 1;
		}

	if (fs->fs_clean == 0)
		fs->fs_flags |= FS_UNCLEAN;

	fs->fs_ronly = ronly;
	size = fs->fs_cssize;
	blks = howmany(size, fs->fs_fsize);
	if (fs->fs_contigsumsize > 0)
		size += fs->fs_ncg * sizeof(int32_t);
	space = malloc((u_long)size, M_UFSMNT, M_WAITOK);
	fs->fs_csp = (struct csum *)space;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + i), size, &bp);
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
			goto out;
		}
		memcpy(space, bp->b_data, size);
		space += size;
		brelse(bp);
		bp = NULL;
	}
	if (fs->fs_contigsumsize > 0) {
		fs->fs_maxcluster = lp = (int32_t *)space;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}
	mp->mnt_data = (qaddr_t)ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	/* Use on-disk fsid if it exists, else fake it */
	if (fs->fs_id[0] != 0 && fs->fs_id[1] != 0)
		mp->mnt_stat.f_fsid.val[1] = fs->fs_id[1];
	else
		mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	mp->mnt_flag |= MNT_LOCAL;
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_nindir = fs->fs_nindir;
	ump->um_bptrtodb = fs->fs_fsbtodb;
	ump->um_seqinc = fs->fs_frag;
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;

	devvp->v_specmountpoint = mp;
	ffs_oldfscompat(fs);

	if (ronly)
		fs->fs_contigdirs = NULL;
	else {
		fs->fs_contigdirs = malloc((u_long)fs->fs_ncg,
		    M_UFSMNT, M_WAITOK|M_ZERO);
	}

	/*
	 * Set FS local "last mounted on" information (NULL pad)
	 */
	memset(fs->fs_fsmnt, 0, sizeof(fs->fs_fsmnt));
	strlcpy(fs->fs_fsmnt, mp->mnt_stat.f_mntonname, sizeof(fs->fs_fsmnt));

#if 0
	if( mp->mnt_flag & MNT_ROOTFS) {
		/*
		 * Root mount; update timestamp in mount structure.
		 * this will be used by the common root mount code
		 * to update the system clock.
		 */
		mp->mnt_time = fs->fs_time;
	}
#endif

	/*
	 * XXX
	 * Limit max file size.  Even though ffs can handle files up to 16TB,
	 * we do limit the max file to 2^31 pages to prevent overflow of
	 * a 32-bit unsigned int.  The buffer cache has its own checks but
	 * a little added paranoia never hurts.
	 */
	ump->um_savedmaxfilesize = fs->fs_maxfilesize;		/* XXX */
	maxfilesize = FS_KERNMAXFILESIZE(PAGE_SIZE, fs);
	if (fs->fs_maxfilesize > maxfilesize)			/* XXX */
		fs->fs_maxfilesize = maxfilesize;		/* XXX */
	if (ronly == 0) {
#ifdef WAPBL
		KASSERT(fs->fs_ronly == 0);
		/*
		 * verify that we can access the last block in the fs if we're
		 * mounting read/write.
		 */
		error = bread(devvp, fsbtodb(fs, fs->fs_size - 1),
		    fs->fs_fsize, &bp);
		if (bp->b_bcount != fs->fs_fsize)
			error = EINVAL;
		bp->b_flags |= B_INVAL;
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
			free(fs->fs_contigdirs, M_UFSMNT);
			goto out;
		}
		brelse(bp);
		bp = NULL;
		/*
		 * ffs_wapbl_start() needs mp->mnt_stat initialised if it
		 * needs to create a new log file in-filesystem.
		 */
		ffs_statfs(mp, &mp->mnt_stat, curproc);

		error = ffs_wapbl_start(mp);
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
			free(fs->fs_contigdirs, M_UFSMNT);
			goto out;
		}
#endif /* WAPBL */
		if ((fs->fs_flags & FS_DOSOFTDEP) &&
		    (error = softdep_mount(devvp, mp, fs, cred)) != 0) {
			free(fs->fs_csp, M_UFSMNT);
			free(fs->fs_contigdirs, M_UFSMNT);
			goto out;
		}
		fs->fs_fmod = 1;
		fs->fs_clean = 0;
		if (mp->mnt_flag & MNT_SOFTDEP)
			fs->fs_flags |= FS_DOSOFTDEP;
		else
			fs->fs_flags &= ~FS_DOSOFTDEP;
		error = UFS_WAPBL_BEGIN(mp);
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
			free(fs->fs_contigdirs, M_UFSMNT);
			goto out;
		}
		error = ffs_sbupdate(ump, MNT_WAIT);
		UFS_WAPBL_END(mp);
		if (error == EROFS) {
			free(fs->fs_csp, M_UFSMNT);
			free(fs->fs_contigdirs, M_UFSMNT);
			goto out;
		}
	}
	return (0);
out:
#ifdef WAPBL
	if (mp->mnt_wapbl_replay) {
		wapbl_replay_stop(mp->mnt_wapbl_replay);
		wapbl_replay_free(mp->mnt_wapbl_replay);
		mp->mnt_wapbl_replay = NULL;
	}
#endif
	devvp->v_specmountpoint = NULL;
	if (bp)
		brelse(bp);

	vn_lock(devvp, LK_EXCLUSIVE|LK_RETRY, p);
	(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, cred);
	VOP_UNLOCK(devvp, 0);

	if (ump) {
		free(ump->um_fs, M_UFSMNT);
		free(ump, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * Sanity checks for old file systems.
 */
int
ffs_oldfscompat(struct fs *fs)
{
	int i;

	fs->fs_npsect = max(fs->fs_npsect, fs->fs_nsect);	/* XXX */
	fs->fs_interleave = max(fs->fs_interleave, 1);		/* XXX */
	if (fs->fs_postblformat == FS_42POSTBLFMT)		/* XXX */
		fs->fs_nrpos = 8;				/* XXX */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		u_int64_t sizepb = fs->fs_bsize;		/* XXX */
								/* XXX */
		fs->fs_maxfilesize = fs->fs_bsize * NDADDR - 1;	/* XXX */
		for (i = 0; i < NIADDR; i++) {			/* XXX */
			sizepb *= NINDIR(fs);			/* XXX */
			fs->fs_maxfilesize += sizepb;		/* XXX */
		}						/* XXX */
		fs->fs_qbmask = ~fs->fs_bmask;			/* XXX */
		fs->fs_qfmask = ~fs->fs_fmask;			/* XXX */
	}							/* XXX */
	if (fs->fs_avgfilesize <= 0)				/* XXX */
		fs->fs_avgfilesize = AVFILESIZ;			/* XXX */
	if (fs->fs_avgfpdir <= 0)				/* XXX */
		fs->fs_avgfpdir = AFPDIR;			/* XXX */
	return (0);
}

/*
 * Auxiliary function for reading FFS1 super blocks.
 */
void
ffs1_compat_read(struct fs *fs, struct ufsmount *ump, daddr_t sbloc)
{
	if (fs->fs_magic == FS_UFS2_MAGIC)
		return; /* UFS2 */
#if 0
	if (fs->fs_ffs1_flags & FS_FLAGS_UPDATED)
		return; /* Already updated */
#endif
	fs->fs_flags = fs->fs_ffs1_flags;
	fs->fs_sblockloc = sbloc;
	fs->fs_maxbsize = fs->fs_bsize;
	fs->fs_time = fs->fs_ffs1_time;
	fs->fs_size = fs->fs_ffs1_size;
	fs->fs_dsize = fs->fs_ffs1_dsize;
	fs->fs_csaddr = fs->fs_ffs1_csaddr;
	fs->fs_cstotal.cs_ndir = fs->fs_ffs1_cstotal.cs_ndir;
	fs->fs_cstotal.cs_nbfree = fs->fs_ffs1_cstotal.cs_nbfree;
	fs->fs_cstotal.cs_nifree = fs->fs_ffs1_cstotal.cs_nifree;
	fs->fs_cstotal.cs_nffree = fs->fs_ffs1_cstotal.cs_nffree;
	fs->fs_ffs1_flags |= FS_FLAGS_UPDATED;
}

/*
 * Auxiliary function for writing FFS1 super blocks.
 */
void
ffs1_compat_write(struct fs *fs, struct ufsmount *ump)
{
	if (fs->fs_magic != FS_UFS1_MAGIC)
		return; /* UFS2 */

	fs->fs_ffs1_time = fs->fs_time;
	fs->fs_ffs1_cstotal.cs_ndir = fs->fs_cstotal.cs_ndir;
	fs->fs_ffs1_cstotal.cs_nbfree = fs->fs_cstotal.cs_nbfree;
	fs->fs_ffs1_cstotal.cs_nifree = fs->fs_cstotal.cs_nifree;
	fs->fs_ffs1_cstotal.cs_nffree = fs->fs_cstotal.cs_nffree;
}

/*
 * unmount system call
 */
int
ffs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	struct ufsmount *ump;
	struct fs *fs;
	int error, flags;
#ifdef WAPBL
	extern int doforce;
#endif

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	if (mp->mnt_flag & MNT_SOFTDEP)
		error = softdep_flushfiles(mp, flags, p);
	else
		error = ffs_flushfiles(mp, flags, p);
	if (error != 0)
		return (error);
	error = UFS_WAPBL_BEGIN(mp);
	if (error)
		goto logfail;
	if (fs->fs_ronly == 0) {
		fs->fs_clean = (fs->fs_flags & FS_UNCLEAN) ? 0 : 1;
		error = ffs_sbupdate(ump, MNT_WAIT);
		/* ignore write errors if mounted RW on read-only device */
		if (error && error != EROFS) {
			fs->fs_clean = 0;
			return (error);
		}
		free(fs->fs_contigdirs, M_UFSMNT);
	}
	UFS_WAPBL_END(mp);
logfail:
#ifdef WAPBL
	KASSERT(!(mp->mnt_wapbl_replay && mp->mnt_wapbl));
	if (mp->mnt_wapbl_replay) {
		KASSERT(fs->fs_ronly);
		wapbl_replay_stop(mp->mnt_wapbl_replay);
		wapbl_replay_free(mp->mnt_wapbl_replay);
		mp->mnt_wapbl_replay = NULL;
	}
	error = ffs_wapbl_stop(mp, doforce && (mntflags & MNT_FORCE));
	if (error) {
		return error;
	}
#endif /* WAPBL */
	ump->um_devvp->v_specmountpoint = NULL;

	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY, p);
	vinvalbuf(ump->um_devvp, V_SAVE, NOCRED, p, 0, 0);
	error = VOP_CLOSE(ump->um_devvp, fs->fs_ronly ? FREAD : FREAD|FWRITE,
		NOCRED);
	vput(ump->um_devvp);
	free(fs->fs_csp, M_UFSMNT);
	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Flush out all the files in a filesystem.
 */
int
ffs_flushfiles(struct mount *mp, int flags, struct proc *p)
{
	struct ufsmount *ump;
	int error;

	ump = VFSTOUFS(mp);
	if (mp->mnt_flag & MNT_QUOTA) {
		int i;
		if ((error = vflush(mp, NULLVP, SKIPSYSTEM|flags)) != 0)
			return (error);
		for (i = 0; i < MAXQUOTAS; i++) {
			if (ump->um_quotas[i] == NULLVP)
				continue;
			quotaoff(p, mp, i);
		}
		/*
		 * Here we fall through to vflush again to ensure
		 * that we have gotten rid of all the system vnodes.
		 */
	}

	/*
	 * Flush all the files.
	 */
	if ((error = vflush(mp, NULL, flags)) != 0)
		return (error);
	/*
	 * Flush filesystem metadata.
	 */
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY, p);
	error = VOP_FSYNC(ump->um_devvp, p->p_ucred, MNT_WAIT);
	VOP_UNLOCK(ump->um_devvp, 0);

#ifdef WAPBL
	if (error)
		return error;
	if (mp->mnt_wapbl) {
		error = wapbl_flush(mp->mnt_wapbl, 1);
		if (flags & FORCECLOSE)
			error = 0;
	}
#endif

	return (error);
}

/*
 * Get file system statistics.
 */
int
ffs_statfs(struct mount *mp, struct statfs *sbp, struct proc *p)
{
	struct ufsmount *ump;
	struct fs *fs;

	ump = VFSTOUFS(mp);
	fs = ump->um_fs;

	switch (fs->fs_magic) {
	case FS_UFS1_MAGIC:
		mp->mnt_stat.mount_info.ufs_args.flags = 0;
		break;
#ifdef FFS2
	case FS_UFS2_MAGIC:
		mp->mnt_stat.mount_info.ufs_args.flags = UFSMNT_FFS2;
		break;
#endif
	default:
		panic("ffs_statfs: unknown magic 0x%x", fs->fs_magic);
	}

	sbp->f_bsize = fs->fs_fsize;
	sbp->f_iosize = fs->fs_bsize;
	sbp->f_blocks = fs->fs_dsize;
	sbp->f_bfree = fs->fs_cstotal.cs_nbfree * fs->fs_frag +
	    fs->fs_cstotal.cs_nffree;
	sbp->f_bavail = sbp->f_bfree -
	    ((int64_t)fs->fs_dsize * fs->fs_minfree / 100);
	sbp->f_files = fs->fs_ncg * fs->fs_ipg - ROOTINO;
	sbp->f_ffree = fs->fs_cstotal.cs_nifree;
	sbp->f_favail = sbp->f_ffree;
	copy_statfs_info(sbp, mp);

	return (0);
}

struct ffs_sync_args {
	int allerror;
	struct proc *p;
	int waitfor;
	struct ucred *cred;
};

int
ffs_sync_vnode(struct vnode *vp, void *arg) {
	struct ffs_sync_args *fsa = arg;
	struct inode *ip;
	int error;

	ip = VTOI(vp);

	/* Skip the vnode/inode if inacessible. */
	if (ip == NULL || vp->v_type == VNON || vp->v_flag & VXLOCK)
		return (0);

	/* Skip the vnode/inode if lazy/clean and nothing to sync. */
	if ((fsa->waitfor == MNT_LAZY || LIST_EMPTY(&vp->v_dirtyblkhd)) &&
	    !(ip->i_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFIED)))
	    	return (0);

	error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT, fsa->p);
	if (error) {
		if (error == ENOENT)
			return (error); /* Force restart of traversal. */
		return (0);
	}

	/*
	 * We deliberately update inode times here.  This will
	 * prevent a massive queue of updates accumulating, only
	 * to be handled by a call to unmount.
	 *
	 * XXX It would be better to have the syncer trickle these
	 * out.  Adjustment needed to allow registering vnodes for
	 * sync when the vnode is clean, but the inode dirty.  Or
	 * have ufs itself trickle out inode updates.
	 *
	 * If doing a lazy sync, we don't care about metadata or
	 * data updates, because they are handled by each vnode's
	 * synclist entry.  In this case we are only interested in
	 * writing back modified inodes.
	 */
	if (fsa->waitfor == MNT_LAZY) {
		error = UFS_WAPBL_BEGIN(vp->v_mount);
		if (!error) {
			error = UFS_UPDATE(ip, 0);
			UFS_WAPBL_END(vp->v_mount);
		}
	} else
		error = VOP_FSYNC(vp, fsa->cred, fsa->waitfor);

	if (error)
		fsa->allerror = error;

	VOP_UNLOCK(vp, 0);
	vrele(vp);

	return (0);
}

/*
 * Go through the disk queues to initiate sandbagged IO;
 * go through the inodes to write those that have been modified;
 * initiate the writing of the super block if it has been modified.
 *
 * Should always be called with the mount point locked.
 */
int
ffs_sync(struct mount *mp, int waitfor, struct ucred *cred, struct proc *p)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs;
	int error, allerror = 0, count;
	struct ffs_sync_args fsa;

	fs = ump->um_fs;
	if (fs->fs_fmod != 0 && fs->fs_ronly != 0) {
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("update: rofs mod");
	}
 loop:
	/*
	 * Write back each (modified) inode.
	 */

	fsa.allerror = 0;
	fsa.p = p;
	fsa.cred = cred;
	fsa.waitfor = waitfor;

	vfs_mount_foreach_vnode(mp, ffs_sync_vnode, &fsa);

	if (fsa.allerror != 0)
		allerror = fsa.allerror;

	/*
	 * Force stale file system control information to be flushed.
	 */
	if ((ump->um_mountp->mnt_flag & MNT_SOFTDEP) && waitfor == MNT_WAIT) {
		if ((error = softdep_flushworklist(ump->um_mountp, &count, p)))
			allerror = error;
		/* Flushed work items may create new vnodes to clean */
		if (count) 
			goto loop;
	}
	if (waitfor != MNT_LAZY) {
		vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY, p);
		if ((error = VOP_FSYNC(ump->um_devvp, cred, waitfor)) != 0)
			allerror = error;
		VOP_UNLOCK(ump->um_devvp, 0);
	}
	qsync(mp);
	/*
	 * Write back modified superblock.
	 */
	if (fs->fs_fmod != 0) {
		error = UFS_WAPBL_BEGIN(mp);
		if (error)
			allerror = error;
		else {
			error = ffs_cgupdate(ump, waitfor);
			if (error)
				allerror = error;
			UFS_WAPBL_END(mp);;
		}
	}


#ifdef WAPBL
	if (mp->mnt_wapbl) {
		error = wapbl_flush(mp->mnt_wapbl, 0);
		if (error)
			allerror = error;
	}
#endif

	return (allerror);
}

/*
 * Look up a FFS dinode number to find its incore vnode, otherwise read it
 * in from disk.  If it is in core, wait for the lock bit to clear, then
 * return the inode locked.  Detection and handling of mount points must be
 * done by the calling routine.
 */
int
ffs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	struct fs *fs;
	struct inode *ip;
	struct ufs1_dinode *dp1;
#ifdef FFS2
	struct ufs2_dinode *dp2;
#endif
	struct ufsmount *ump;
	struct buf *bp;
	struct vnode *vp;
	dev_t dev;
	int error;

	if (ino > (ufsino_t)-1)
		panic("ffs_vget: alien ino_t %llu", (unsigned long long)ino);

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
retry:
	if ((*vpp = ufs_ihashget(dev, ino)) != NULL)
		return (0);

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_UFS, mp, &ffs_vops, &vp)) != 0) {
		*vpp = NULL;
		return (error);
	}

#ifdef VFSLCKDEBUG
	vp->v_flag |= VLOCKSWORK;
#endif
	ip = pool_get(&ffs_ino_pool, PR_WAITOK|PR_ZERO);
	lockinit(&ip->i_lock, PINOD, "inode", 0, 0);
	ip->i_ump = ump;
	vref(ip->i_devvp);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_fs = fs = ump->um_fs;
	ip->i_dev = dev;
	ip->i_number = ino;
	ip->i_vtbl = &ffs_vtbl;

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	error = ufs_ihashins(ip);
	
	if (error) {
		/*
		 * Inode has not been inserted into the chain, so make sure
		 * we don't try to remove it.
		 */
		ip->i_flag |= IN_UNHASHED;

		/*
		 * VOP_INACTIVE will treat this as a stale file
		 * and recycle it quickly
		 */
		vrele(vp);

		if (error == EEXIST)
			goto retry;

		return (error);
	}


	/* Read in the disk contents for the inode, copy into the inode. */
	error = bread(ump->um_devvp, fsbtodb(fs, ino_to_fsba(fs, ino)),
	    (int)fs->fs_bsize, &bp);
	if (error) {
		/*
		 * The inode does not contain anything useful, so it would
		 * be misleading to leave it on its hash chain. With mode
		 * still zero, it will be unlinked and returned to the free
		 * list by vput().
		 */
		vput(vp);
		brelse(bp);
		*vpp = NULL;
		return (error);
	}

#ifdef FFS2
	if (ip->i_ump->um_fstype == UM_UFS2) {
		ip->i_din2 = pool_get(&ffs_dinode2_pool, PR_WAITOK);
		dp2 = (struct ufs2_dinode *) bp->b_data + ino_to_fsbo(fs, ino);
		*ip->i_din2 = *dp2;
	} else
#endif
	{
		ip->i_din1 = pool_get(&ffs_dinode1_pool, PR_WAITOK);
		dp1 = (struct ufs1_dinode *) bp->b_data + ino_to_fsbo(fs, ino);
		*ip->i_din1 = *dp1;
	}

	brelse(bp);

	if (DOINGSOFTDEP(vp))
		softdep_load_inodeblock(ip);
	else
		ip->i_effnlink = DIP(ip, nlink);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */
	error = ufs_vinit(mp, &ffs_specvops, FFS_FIFOOPS, &vp);
	if (error) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}

	/*
	 * Set up a generation number for this inode if it does not
	 * already have one. This should only happen on old filesystems.
	 */
	if (DIP(ip, gen) == 0) {
		DIP_ASSIGN(ip, gen, arc4random() & INT_MAX);
		if (DIP(ip, gen) == 0 || DIP(ip, gen) == -1)
			DIP_ASSIGN(ip, gen, 1);	/* Shouldn't happen */
		if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0)
			ip->i_flag |= IN_MODIFIED;
	}

	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
	if (fs->fs_magic == FS_UFS1_MAGIC && fs->fs_inodefmt < FS_44INODEFMT) {
		ip->i_ffs1_uid = ip->i_din1->di_ouid;
		ip->i_ffs1_gid = ip->i_din1->di_ogid;
	}

	*vpp = vp;

	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is valid
 * - call ffs_vget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 */
int
ffs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	struct ufid *ufhp;
	struct fs *fs;

	ufhp = (struct ufid *)fhp;
	fs = VFSTOUFS(mp)->um_fs;
	if (ufhp->ufid_len != sizeof(*ufhp) ||
	    ufhp->ufid_ino < ROOTINO ||
	    ufhp->ufid_ino >= fs->fs_ncg * fs->fs_ipg)
		return (ESTALE);
	return (ufs_fhtovp(mp, ufhp, vpp));
}

/*
 * Vnode pointer to File handle
 */
int
ffs_vptofh(struct vnode *vp, struct fid *fhp)
{
	struct inode *ip;
	struct ufid *ufhp;

	ip = VTOI(vp);
	ufhp = (struct ufid *)fhp;
	ufhp->ufid_len = sizeof(struct ufid);
	ufhp->ufid_ino = ip->i_number;
	ufhp->ufid_gen = DIP(ip, gen);

	return (0);
}

/*
 * Write a superblock and associated information back to disk.
 */
int
ffs_sbupdate(struct ufsmount *mp, int waitfor)
{
	struct fs *dfs, *fs = mp->um_fs;
	struct buf *bp;
	int blks;
	caddr_t space;
	int i, size, error, allerror = 0;

	/*
	 * First write back the summary information.
	 */
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->um_devvp, fsbtodb(fs, fs->fs_csaddr + i),
			    size, 0, 0);
		memcpy(bp->b_data, space, size);
		space += size;
		if (waitfor != MNT_WAIT)
			bawrite(bp);
		else if ((error = bwrite(bp)))
			allerror = error;
	}

	/*
	 * Now write back the superblock itself. If any errors occurred
	 * up to this point, then fail so that the superblock avoids
	 * being written out as clean.
	 */
	if (allerror) {
		return (allerror);
	}

	bp = getblk(mp->um_devvp,
	    fs->fs_sblockloc >> (fs->fs_fshift - fs->fs_fsbtodb),
	    (int)fs->fs_sbsize, 0, 0);
	fs->fs_fmod = 0;
	fs->fs_time = time_second;
	memcpy(bp->b_data, fs, fs->fs_sbsize);
	/* Restore compatibility to old file systems.		   XXX */
	dfs = (struct fs *)bp->b_data;				/* XXX */
	if (fs->fs_postblformat == FS_42POSTBLFMT)		/* XXX */
		dfs->fs_nrpos = -1;				/* XXX */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		int32_t *lp, tmp;				/* XXX */
								/* XXX */
		lp = (int32_t *)&dfs->fs_qbmask;		/* XXX */
		tmp = lp[4];					/* XXX */
		for (i = 4; i > 0; i--)				/* XXX */
			lp[i] = lp[i-1];			/* XXX */
		lp[0] = tmp;					/* XXX */
	}							/* XXX */
	dfs->fs_maxfilesize = mp->um_savedmaxfilesize;		/* XXX */

	ffs1_compat_write(dfs, mp);

	if (waitfor != MNT_WAIT)
		bawrite(bp);
	else if ((error = bwrite(bp)))
		allerror = error;

	return (allerror);
}

int
ffs_cgupdate(struct ufsmount *mp, int waitfor)
{
	struct fs *fs = mp->um_fs;
	struct buf *bp;
	int blks;
	void *space;
	int i, size, error = 0, allerror = 0;

	allerror = ffs_sbupdate(mp, waitfor);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = fs->fs_csp;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->um_devvp, fsbtodb(fs, fs->fs_csaddr + i), size,
		    0, 0);
#ifdef FFS_EI
		if (mp->um_flags & UFS_NEEDSWAP)
			ffs_csum_swap((struct csum*)space,
			    (struct csum*)bp->b_data, size);
		else
#endif
			memcpy(bp->b_data, space, (u_int)size);
		space = (char *)space + size;
		if (waitfor == MNT_WAIT)
			error = bwrite(bp);
		else
			bawrite(bp);
	}
	if (!allerror && error)
		allerror = error;
	return (allerror);
}

int
ffs_init(struct vfsconf *vfsp)
{
	static int done;

	if (done)
		return (0);

	done = 1;

	pool_init(&ffs_ino_pool, sizeof(struct inode), 0, 0, 0, "ffsino",
	    &pool_allocator_nointr);
	pool_init(&ffs_dinode1_pool, sizeof(struct ufs1_dinode), 0, 0, 0,
	    "dino1pl", &pool_allocator_nointr);
#ifdef FFS2
	pool_init(&ffs_dinode2_pool, sizeof(struct ufs2_dinode), 0, 0, 0,
	    "dino2pl", &pool_allocator_nointr);
#endif

	softdep_initialize();

	return (ufs_init(vfsp));
}

/*
 * fast filesystem related variables.
 */
int
ffs_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp, void *newp,
    size_t newlen, struct proc *p)
{
	extern int doclusterread, doclusterwrite, doreallocblks, doasyncfree;
#ifdef FFS_SOFTUPDATES
	extern int max_softdeps, tickdelay, stat_worklist_push;
	extern int stat_blk_limit_push, stat_ino_limit_push, stat_blk_limit_hit;
	extern int stat_ino_limit_hit, stat_sync_limit_hit, stat_indir_blk_ptrs;
	extern int stat_inode_bitmap, stat_direct_blk_ptrs, stat_dir_entry;
#endif

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case FFS_CLUSTERREAD:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doclusterread));
	case FFS_CLUSTERWRITE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doclusterwrite));
	case FFS_REALLOCBLKS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doreallocblks));
	case FFS_ASYNCFREE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &doasyncfree));
#ifdef FFS_SOFTUPDATES
	case FFS_MAX_SOFTDEPS:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &max_softdeps));
	case FFS_SD_TICKDELAY:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &tickdelay));
	case FFS_SD_WORKLIST_PUSH:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_worklist_push));
	case FFS_SD_BLK_LIMIT_PUSH:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_blk_limit_push));
	case FFS_SD_INO_LIMIT_PUSH:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_ino_limit_push));
	case FFS_SD_BLK_LIMIT_HIT:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_blk_limit_hit));
	case FFS_SD_INO_LIMIT_HIT:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_ino_limit_hit));
	case FFS_SD_SYNC_LIMIT_HIT:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_sync_limit_hit));
	case FFS_SD_INDIR_BLK_PTRS:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_indir_blk_ptrs));
	case FFS_SD_INODE_BITMAP:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_inode_bitmap));
	case FFS_SD_DIRECT_BLK_PTRS:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_direct_blk_ptrs));
	case FFS_SD_DIR_ENTRY:
		return (sysctl_rdint(oldp, oldlenp, newp, stat_dir_entry));
#endif
#ifdef UFS_DIRHASH
	case FFS_DIRHASH_DIRSIZE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ufs_mindirhashsize));
	case FFS_DIRHASH_MAXMEM:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ufs_dirhashmaxmem));
	case FFS_DIRHASH_MEM:
		return (sysctl_rdint(oldp, oldlenp, newp, ufs_dirhashmem));
#endif

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
