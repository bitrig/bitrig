/*	$OpenBSD: vfs_syscalls.c,v 1.154 2009/06/03 03:57:20 blambert Exp $	*/
/*	$NetBSD: vfs_syscalls.c,v 1.71 1996/04/23 10:29:02 mycroft Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_syscalls.c	8.28 (Berkeley) 12/10/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/dirent.h>
#include <sys/dkio.h>
#include <sys/disklabel.h>

#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

extern int suid_clear;
int	usermount = 0;		/* sysctl: by default, users may not mount */

static int change_dir(struct nameidata *, struct proc *);

void checkdirs(struct vnode *);

int copyout_statfs(struct statfs *, void *, struct proc *);

/*
 * Virtual File System System Calls
 */

/*
 * Mount a file system.
 */
/* ARGSUSED */
int
sys_mount(struct proc *p, void *v, register_t *retval)
{
	struct sys_mount_args /* {
		syscallarg(const char *) type;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */ *uap = v;
	struct vnode *vp;
	struct mount *mp;
	int error, flag = 0;
#ifdef COMPAT_43
	u_long fstypenum = 0;
#endif
	char fstypename[MFSNAMELEN];
	char fspath[MNAMELEN];
	struct vattr va;
	struct nameidata nd;
	struct vfsconf *vfsp;

	if (usermount == 0 && (error = suser(p, 0)))
		return (error);

	/*
	 * Mount points must fit in MNAMELEN, not MAXPATHLEN.
	 */
	error = copyinstr(SCARG(uap, path), fspath, MNAMELEN, NULL);
	if (error)
		return(error);

	/*
	 * Get vnode to be covered
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, fspath, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (SCARG(uap, flags) & MNT_UPDATE) {
		if ((vp->v_flag & VROOT) == 0) {
			vput(vp);
			return (EINVAL);
		}
		mp = vp->v_mount;
		flag = mp->mnt_flag;
		/*
		 * We only allow the filesystem to be reloaded if it
		 * is currently mounted read-only.
		 */
		if ((SCARG(uap, flags) & MNT_RELOAD) &&
		    ((mp->mnt_flag & MNT_RDONLY) == 0)) {
			vput(vp);
			return (EOPNOTSUPP);	/* Needs translation */
		}

		/*
		 * Only root, or the user that did the original mount is
		 * permitted to update it.
		 */
		if (mp->mnt_stat.f_owner != p->p_ucred->cr_uid &&
		    (error = suser(p, 0))) {
			vput(vp);
			return (error);
		}
		/*
		 * Do not allow NFS export by non-root users. Silently
		 * enforce MNT_NOSUID and MNT_NODEV for non-root users, and
		 * inherit MNT_NOEXEC from the mount point.
		 */
		if (p->p_ucred->cr_uid != 0) {
			if (SCARG(uap, flags) & MNT_EXPORTED) {
				vput(vp);
				return (EPERM);
			}
			SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
			if (flag & MNT_NOEXEC)
				SCARG(uap, flags) |= MNT_NOEXEC;
		}
		if ((error = vfs_busy(mp, VB_READ|VB_NOWAIT)) != 0) {
			vput(vp);
			return (error);
		}
		mp->mnt_flag |= SCARG(uap, flags) & (MNT_RELOAD | MNT_UPDATE);
		goto update;
	}
	/*
	 * If the user is not root, ensure that they own the directory
	 * onto which we are attempting to mount.
	 */
	if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)) ||
	    (va.va_uid != p->p_ucred->cr_uid &&
	    (error = suser(p, 0)))) {
		vput(vp);
		return (error);
	}
	/*
	 * Do not allow NFS export by non-root users. Silently
	 * enforce MNT_NOSUID and MNT_NODEV for non-root users, and inherit
	 * MNT_NOEXEC from the mount point.
	 */
	if (p->p_ucred->cr_uid != 0) {
		if (SCARG(uap, flags) & MNT_EXPORTED) {
			vput(vp);
			return (EPERM);
		}
		SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
		if (vp->v_mount->mnt_flag & MNT_NOEXEC)
			SCARG(uap, flags) |= MNT_NOEXEC;
	}
	if ((error = vinvalbuf(vp, V_SAVE, p->p_ucred, p, 0, 0)) != 0) {
		vput(vp);
		return (error);
	}
	if (vp->v_type != VDIR) {
		vput(vp);
		return (ENOTDIR);
	}
	error = copyinstr(SCARG(uap, type), fstypename, MFSNAMELEN, NULL);
	if (error) {
#ifdef COMPAT_43
		/*
		 * Historically filesystem types were identified by number.
		 * If we get an integer for the filesystem type instead of a
		 * string, we check to see if it matches one of the historic
		 * filesystem types.
		 */
		fstypenum = (u_long)SCARG(uap, type);

		for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next)
			if (vfsp->vfc_typenum == fstypenum)
				break;
		if (vfsp == NULL) {
			vput(vp);
			return (ENODEV);
		}
		strncpy(fstypename, vfsp->vfc_name, MFSNAMELEN);

#else
		vput(vp);
		return (error);
#endif
	}
	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (!strcmp(vfsp->vfc_name, fstypename))
			break;
	}

	if (vfsp == NULL) {
		vput(vp);
		return (EOPNOTSUPP);
	}

	if (vp->v_mountedhere != NULL) {
		vput(vp);
		return (EBUSY);
	}

	/*
	 * Allocate and initialize the file system.
	 */
	mp = (struct mount *)malloc((u_long)sizeof(struct mount),
		M_MOUNT, M_WAITOK|M_ZERO);
	(void) vfs_busy(mp, VB_READ|VB_NOWAIT);
	mp->mnt_op = vfsp->vfc_vfsops;
	mp->mnt_vfc = vfsp;
	mp->mnt_flag |= (vfsp->vfc_flags & MNT_VISFLAGMASK);
	strncpy(mp->mnt_stat.f_fstypename, vfsp->vfc_name, MFSNAMELEN);
	mp->mnt_vnodecovered = vp;
	mp->mnt_stat.f_owner = p->p_ucred->cr_uid;
update:
	/*
	 * Set the mount level flags.
	 */
	if (SCARG(uap, flags) & MNT_RDONLY)
		mp->mnt_flag |= MNT_RDONLY;
	else if (mp->mnt_flag & MNT_RDONLY)
		mp->mnt_flag |= MNT_WANTRDWR;
	mp->mnt_flag &=~ (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_ASYNC | MNT_SOFTDEP | MNT_NOATIME |
	    MNT_FORCE);
	mp->mnt_flag |= SCARG(uap, flags) & (MNT_NOSUID | MNT_NOEXEC |
	    MNT_NODEV | MNT_SYNCHRONOUS | MNT_ASYNC | MNT_SOFTDEP |
	    MNT_NOATIME | MNT_FORCE);
	/*
	 * Mount the filesystem.
	 */
	error = VFS_MOUNT(mp, SCARG(uap, path), SCARG(uap, data), &nd, p);
	if (!error) {
		mp->mnt_stat.f_ctime = time_second;
	}
	if (mp->mnt_flag & MNT_UPDATE) {
		vput(vp);
		if (mp->mnt_flag & MNT_WANTRDWR)
			mp->mnt_flag &= ~MNT_RDONLY;
		mp->mnt_flag &=~
		    (MNT_UPDATE | MNT_RELOAD | MNT_FORCE | MNT_WANTRDWR);
		if (error)
			mp->mnt_flag = flag;

 		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
 			if (mp->mnt_syncer == NULL)
 				error = vfs_allocate_syncvnode(mp);
 		} else {
 			if (mp->mnt_syncer != NULL)
 				vgone(mp->mnt_syncer);
 			mp->mnt_syncer = NULL;
 		}

		vfs_unbusy(mp);
		return (error);
	}

	vp->v_mountedhere = mp;

	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(vp);
	if (!error) {
		vfsp->vfc_refcount++;
		CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
		checkdirs(vp);
		VOP_UNLOCK(vp, 0, p);
 		if ((mp->mnt_flag & MNT_RDONLY) == 0)
 			error = vfs_allocate_syncvnode(mp);
		vfs_unbusy(mp);
		(void) VFS_STATFS(mp, &mp->mnt_stat, p);
		if ((error = VFS_START(mp, 0, p)) != 0)
			vrele(vp);
	} else {
		mp->mnt_vnodecovered->v_mountedhere = (struct mount *)0;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vput(vp);
	}
	return (error);
}

/*
 * Scan all active processes to see if any of them have a current
 * or root directory onto which the new filesystem has just been
 * mounted. If so, replace them with the new mount point.
 */
void
checkdirs(struct vnode *olddp)
{
	struct filedesc *fdp;
	struct vnode *newdp, *vp;
	struct proc *p;

	if (olddp->v_usecount == 1)
		return;
	if (VFS_ROOT(olddp->v_mountedhere, &newdp))
		panic("mount: lost mount");
again:
	LIST_FOREACH(p, &allproc, p_list) {
		fdp = p->p_fd;
		if (fdp->fd_cdir == olddp) {
			vp = fdp->fd_cdir;
			VREF(newdp);
			fdp->fd_cdir = newdp;
			if (vrele(vp))
				goto again;
		}
		if (fdp->fd_rdir == olddp) {
			vp = fdp->fd_rdir;
			VREF(newdp);
			fdp->fd_rdir = newdp;
			if (vrele(vp))
				goto again;
		}
	}
	if (rootvnode == olddp) {
		vrele(rootvnode);
		VREF(newdp);
		rootvnode = newdp;
	}
	vput(newdp);
}

/*
 * Unmount a file system.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
/* ARGSUSED */
int
sys_unmount(struct proc *p, void *v, register_t *retval)
{
	struct sys_unmount_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct vnode *vp;
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	mp = vp->v_mount;

	/*
	 * Only root, or the user that did the original mount is
	 * permitted to unmount this filesystem.
	 */
	if ((mp->mnt_stat.f_owner != p->p_ucred->cr_uid) &&
	    (error = suser(p, 0))) {
		vput(vp);
		return (error);
	}

	/*
	 * Don't allow unmounting the root file system.
	 */
	if (mp->mnt_flag & MNT_ROOTFS) {
		vput(vp);
		return (EINVAL);
	}

	/*
	 * Must be the root of the filesystem
	 */
	if ((vp->v_flag & VROOT) == 0) {
		vput(vp);
		return (EINVAL);
	}
	vput(vp);

	if (vfs_busy(mp, VB_WRITE|VB_WAIT))
		return (EBUSY);

	return (dounmount(mp, SCARG(uap, flags), p, vp));
}

/*
 * Do the actual file system unmount.
 */
int
dounmount(struct mount *mp, int flags, struct proc *p, struct vnode *olddp)
{
	struct vnode *coveredvp;
	int error;
	int hadsyncer = 0;

 	mp->mnt_flag &=~ MNT_ASYNC;
 	cache_purgevfs(mp);	/* remove cache entries for this file sys */
 	if (mp->mnt_syncer != NULL) {
		hadsyncer = 1;
 		vgone(mp->mnt_syncer);
		mp->mnt_syncer = NULL;
	}
	if (((mp->mnt_flag & MNT_RDONLY) ||
	    (error = VFS_SYNC(mp, MNT_WAIT, p->p_ucred, p)) == 0) ||
 	    (flags & MNT_FORCE))
 		error = VFS_UNMOUNT(mp, flags, p);

 	if (error && error != EIO && !(flags & MNT_DOOMED)) {
 		if ((mp->mnt_flag & MNT_RDONLY) == 0 && hadsyncer)
 			(void) vfs_allocate_syncvnode(mp);
		vfs_unbusy(mp);
		return (error);
	}

	CIRCLEQ_REMOVE(&mountlist, mp, mnt_list);
	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP) {
		coveredvp->v_mountedhere = NULL;
 		vrele(coveredvp);
 	}

	mp->mnt_vfc->vfc_refcount--;

	if (!LIST_EMPTY(&mp->mnt_vnodelist))
		panic("unmount: dangling vnode");

	vfs_unbusy(mp);
	free(mp, M_MOUNT);

	return (0);
}

/*
 * Sync each mounted filesystem.
 */
#ifdef DEBUG
int syncprt = 0;
struct ctldebug debug0 = { "syncprt", &syncprt };
#endif

/* ARGSUSED */
int
sys_sync(struct proc *p, void *v, register_t *retval)
{
	struct mount *mp, *nmp;
	int asyncflag;

	for (mp = CIRCLEQ_LAST(&mountlist); mp != CIRCLEQ_END(&mountlist);
	    mp = nmp) {
		if (vfs_busy(mp, VB_READ|VB_NOWAIT)) {
			nmp = CIRCLEQ_PREV(mp, mnt_list);
			continue;
		}
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			asyncflag = mp->mnt_flag & MNT_ASYNC;
			mp->mnt_flag &= ~MNT_ASYNC;
			uvm_vnp_sync(mp);
			VFS_SYNC(mp, MNT_NOWAIT, p->p_ucred, p);
			if (asyncflag)
				mp->mnt_flag |= MNT_ASYNC;
		}
		nmp = CIRCLEQ_PREV(mp, mnt_list);
		vfs_unbusy(mp);
	}

	return (0);
}

/*
 * Change filesystem quotas.
 */
/* ARGSUSED */
int
sys_quotactl(struct proc *p, void *v, register_t *retval)
{
	struct sys_quotactl_args /* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(char *) arg;
	} */ *uap = v;
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	vrele(nd.ni_vp);
	return (VFS_QUOTACTL(mp, SCARG(uap, cmd), SCARG(uap, uid),
	    SCARG(uap, arg), p));
}

int
copyout_statfs(struct statfs *sp, void *uaddr, struct proc *p)
{
	size_t co_sz1 = offsetof(struct statfs, f_fsid);
	size_t co_off2 = co_sz1 + sizeof(fsid_t);
	size_t co_sz2 = sizeof(struct statfs) - co_off2;
	char *s, *d;
	int error;

	/* Don't let non-root see filesystem id (for NFS security) */
	if (suser(p, 0)) {
		fsid_t fsid;

		s = (char *)sp;
		d = (char *)uaddr;

		memset(&fsid, 0, sizeof(fsid));

		if ((error = copyout(s, d, co_sz1)) != 0)
			return (error);
		if ((error = copyout(&fsid, d + co_sz1, sizeof(fsid))) != 0)
			return (error);
		return (copyout(s + co_off2, d + co_off2, co_sz2));
	}

	return (copyout(sp, uaddr, sizeof(*sp)));
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
sys_statfs(struct proc *p, void *v, register_t *retval)
{
	struct sys_statfs_args /* {
		syscallarg(const char *) path;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statfs *sp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;

	return (copyout_statfs(sp, SCARG(uap, buf), p));
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
sys_fstatfs(struct proc *p, void *v, register_t *retval)
{
	struct sys_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	int error;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	if (!mp) {
		FRELE(fp);
		return (ENOENT);
	}
	sp = &mp->mnt_stat;
	error = VFS_STATFS(mp, sp, p);
	FRELE(fp);
	if (error)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;

	return (copyout_statfs(sp, SCARG(uap, buf), p));
}

/*
 * Get statistics on all filesystems.
 */
int
sys_getfsstat(struct proc *p, void *v, register_t *retval)
{
	struct sys_getfsstat_args /* {
		syscallarg(struct statfs *) buf;
		syscallarg(size_t) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp, *nmp;
	struct statfs *sp;
	struct statfs *sfsp;
	size_t count, maxcount;
	int error, flags = SCARG(uap, flags);

	maxcount = SCARG(uap, bufsize) / sizeof(struct statfs);
	sfsp = SCARG(uap, buf);
	count = 0;

	for (mp = CIRCLEQ_FIRST(&mountlist); mp != CIRCLEQ_END(&mountlist);
	    mp = nmp) {
		if (vfs_busy(mp, VB_READ|VB_NOWAIT)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if (sfsp && count < maxcount) {
			sp = &mp->mnt_stat;

			/* Refresh stats unless MNT_NOWAIT is specified */
			if (flags != MNT_NOWAIT &&
			    flags != MNT_LAZY &&
			    (flags == MNT_WAIT ||
			    flags == 0) &&
			    (error = VFS_STATFS(mp, sp, p))) {
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				vfs_unbusy(mp);
 				continue;
			}

			sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
#if notyet
			if (mp->mnt_flag & MNT_SOFTDEP)
				sp->f_eflags = STATFS_SOFTUPD;
#endif
			error = (copyout_statfs(sp, sfsp, p));
			if (error) {
				vfs_unbusy(mp);
				return (error);
			}
			sfsp++;
		}
		count++;
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}

	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;

	return (0);
}

/*
 * Change current working directory to a given file descriptor.
 */
/* ARGSUSED */
int
sys_fchdir(struct proc *p, void *v, register_t *retval)
{
	struct sys_fchdir_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct vnode *vp, *tdp;
	struct mount *mp;
	struct file *fp;
	int error;

	if ((error = getvnode(fdp, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	VREF(vp);
	FRELE(fp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);

	while (!error && (mp = vp->v_mountedhere) != NULL) {
		if (vfs_busy(mp, VB_READ|VB_WAIT))
			continue;
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp);
		if (error)
			break;
		vput(vp);
		vp = tdp;
	}
	if (error) {
		vput(vp);
		return (error);
	}
	VOP_UNLOCK(vp, 0, p);
	vrele(fdp->fd_cdir);
	fdp->fd_cdir = vp;
	return (0);
}

/*
 * Change current working directory (``.'').
 */
/* ARGSUSED */
int
sys_chdir(struct proc *p, void *v, register_t *retval)
{
	struct sys_chdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)) != 0)
		return (error);
	vrele(fdp->fd_cdir);
	fdp->fd_cdir = nd.ni_vp;
	return (0);
}

/*
 * Change notion of root (``/'') directory.
 */
/* ARGSUSED */
int
sys_chroot(struct proc *p, void *v, register_t *retval)
{
	struct sys_chroot_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	int error;
	struct nameidata nd;

	if ((error = suser(p, 0)) != 0)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)) != 0)
		return (error);
	if (fdp->fd_rdir != NULL) {
		/*
		 * A chroot() done inside a changed root environment does
		 * an automatic chdir to avoid the out-of-tree experience.
		 */
		vrele(fdp->fd_rdir);
		vrele(fdp->fd_cdir);
		VREF(nd.ni_vp);
		fdp->fd_cdir = nd.ni_vp;
	}
	fdp->fd_rdir = nd.ni_vp;
	return (0);
}

/*
 * Common routine for chroot and chdir.
 */
static int
change_dir(struct nameidata *ndp, struct proc *p)
{
	struct vnode *vp;
	int error;

	if ((error = namei(ndp)) != 0)
		return (error);
	vp = ndp->ni_vp;
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	if (error)
		vput(vp);
	else
		VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
sys_open(struct proc *p, void *v, register_t *retval)
{
	struct sys_open_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct vattr vattr;
	int flags, cmode;
	int type, indx, error, localtrunc = 0;
	struct flock lf;
	struct nameidata nd;

	fdplock(fdp);

	if ((error = falloc(p, &fp, &indx)) != 0)
		goto out;

	flags = FFLAGS(SCARG(uap, flags));
	cmode = ((SCARG(uap, mode) &~ fdp->fd_cmask) & ALLPERMS) &~ S_ISTXT;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	p->p_dupfd = -1;			/* XXX check for fdopen */
	if ((flags & O_TRUNC) && (flags & (O_EXLOCK | O_SHLOCK))) {
		localtrunc = 1;
		flags &= ~O_TRUNC;	/* Must do truncate ourselves */
	}
	if ((error = vn_open(&nd, flags, cmode)) != 0) {
		if ((error == ENODEV || error == ENXIO) &&
		    p->p_dupfd >= 0 &&			/* XXX from fdopen */
		    (error =
			dupfdopen(fdp, indx, p->p_dupfd, flags, error)) == 0) {
			closef(fp, p);
			*retval = indx;
			goto out;
		}
		if (error == ERESTART)
			error = EINTR;
		fdremove(fdp, indx);
		closef(fp, p);
		goto out;
	}
	p->p_dupfd = 0;
	vp = nd.ni_vp;
	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	if (flags & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (flags & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0)
			type |= F_WAIT;
		VOP_UNLOCK(vp, 0, p);
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, type);
		if (error) {
			/* closef will vn_close the file for us. */
			fdremove(fdp, indx);
			closef(fp, p);
			goto out;
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		fp->f_flag |= FHASLOCK;
	}
	if (localtrunc) {
		if ((fp->f_flag & FWRITE) == 0)
			error = EACCES;
		else if (vp->v_mount->mnt_flag & MNT_RDONLY)
			error = EROFS;
		else if (vp->v_type == VDIR)
			error = EISDIR;
		else if ((error = vn_writechk(vp)) == 0) {
			VATTR_NULL(&vattr);
			vattr.va_size = 0;
			error = VOP_SETATTR(vp, &vattr, fp->f_cred, p);
		}
		if (error) {
			VOP_UNLOCK(vp, 0, p);
			/* closef will close the file for us. */
			fdremove(fdp, indx);
			closef(fp, p);
			goto out;
		}
	}
	VOP_UNLOCK(vp, 0, p);
	*retval = indx;
	FILE_SET_MATURE(fp);
out:
	fdpunlock(fdp);
	return (error);
}

/*
 * Get file handle system call
 */
int
sys_getfh(struct proc *p, void *v, register_t *retval)
{
	struct sys_getfh_args /* {
		syscallarg(const char *) fname;
		syscallarg(fhandle_t *) fhp;
	} */ *uap = v;
	struct vnode *vp;
	fhandle_t fh;
	int error;
	struct nameidata nd;

	/*
	 * Must be super user
	 */
	error = suser(p, 0);
	if (error)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, fname), p);
	error = namei(&nd);
	if (error)
		return (error);
	vp = nd.ni_vp;
	bzero(&fh, sizeof(fh));
	fh.fh_fsid = vp->v_mount->mnt_stat.f_fsid;
	error = VFS_VPTOFH(vp, &fh.fh_fid);
	vput(vp);
	if (error)
		return (error);
	error = copyout(&fh, SCARG(uap, fhp), sizeof(fh));
	return (error);
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
sys_fhopen(struct proc *p, void *v, register_t *retval)
{
	struct sys_fhopen_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp = NULL;
	struct mount *mp;
	struct ucred *cred = p->p_ucred;
	int flags;
	int type, indx, error=0;
	struct flock lf;
	struct vattr va;
	fhandle_t fh;

	/*
	 * Must be super user
	 */
	if ((error = suser(p, 0)))
		return (error);

	flags = FFLAGS(SCARG(uap, flags));
	if ((flags & (FREAD | FWRITE)) == 0)
		return (EINVAL);
	if ((flags & O_CREAT))
		return (EINVAL);

	fdplock(fdp);
	if ((error = falloc(p, &fp, &indx)) != 0) {
		fp = NULL;
		goto bad;
	}

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		goto bad;

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL) {
		error = ESTALE;
		goto bad;
	}

	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)) != 0) {
		vp = NULL;	/* most likely unnecessary sanity for bad: */
		goto bad;
	}

	/* Now do an effective vn_open */

	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if (flags & FREAD) {
		if ((error = VOP_ACCESS(vp, VREAD, cred, p)) != 0)
			goto bad;
	}
	if (flags & (FWRITE | O_TRUNC)) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto bad;
		}
		if ((error = vn_writechk(vp)) != 0 ||
		    (error = VOP_ACCESS(vp, VWRITE, cred, p)) != 0)
			goto bad;
	}
	if (flags & O_TRUNC) {
		VATTR_NULL(&va);
		va.va_size = 0;
		if ((error = VOP_SETATTR(vp, &va, cred, p)) != 0)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, flags, cred, p)) != 0)
		goto bad;
	if (flags & FWRITE)
		vp->v_writecount++;

	/* done with modified vn_open, now finish what sys_open does. */

	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	if (flags & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (flags & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0)
			type |= F_WAIT;
		VOP_UNLOCK(vp, 0, p);
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, type);
		if (error) {
			vp = NULL;	/* closef will vn_close the file */
			goto bad;
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		fp->f_flag |= FHASLOCK;
	}
	VOP_UNLOCK(vp, 0, p);
	*retval = indx;
	FILE_SET_MATURE(fp);

	fdpunlock(fdp);
	return (0);

bad:
	if (fp) {
		fdremove(fdp, indx);
		closef(fp, p);
		if (vp != NULL)
			vput(vp);
	}
	fdpunlock(fdp);
	return (error);
}

/* ARGSUSED */
int
sys_fhstat(struct proc *p, void *v, register_t *retval)
{
	struct sys_fhstat_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct stat *) sb;
	} */ *uap = v;
	struct stat sb;
	int error;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = suser(p, 0)))
		return (error);

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb, p);
	vput(vp);
	if (error)
		return (error);
	error = copyout(&sb, SCARG(uap, sb), sizeof(sb));
	return (error);
}

/* ARGSUSED */
int
sys_fhstatfs(struct proc *p, void *v, register_t *retval)
{
	struct sys_fhstatfs_args /*
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct statfs *sp;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = suser(p, 0)))
		return (error);

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	vput(vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return (copyout(sp, SCARG(uap, buf), sizeof(sp)));
}

/*
 * Create a special file.
 */
/* ARGSUSED */
int
sys_mknod(struct proc *p, void *v, register_t *retval)
{
	struct sys_mknod_args /* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	if ((error = suser(p, 0)) != 0)
		return (error);
	if (p->p_fd->fd_rdir)
		return (EINVAL);
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL)
		error = EEXIST;
	else {
		VATTR_NULL(&vattr);
		vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_fd->fd_cmask;
		vattr.va_rdev = SCARG(uap, dev);

		switch (SCARG(uap, mode) & S_IFMT) {
		case S_IFMT:	/* used by badsect to flag bad sectors */
			vattr.va_type = VBAD;
			break;
		case S_IFCHR:
			vattr.va_type = VCHR;
			break;
		case S_IFBLK:
			vattr.va_type = VBLK;
			break;
		default:
			error = EINVAL;
			break;
		}
	}
	if (!error) {
		error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (vp)
			vrele(vp);
	}
	return (error);
}

/*
 * Create a named pipe.
 */
/* ARGSUSED */
int
sys_mkfifo(struct proc *p, void *v, register_t *retval)
{
#ifndef FIFO
	return (EOPNOTSUPP);
#else
	struct sys_mkfifo_args /* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	if (nd.ni_vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		return (EEXIST);
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VFIFO;
	vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_fd->fd_cmask;
	return (VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr));
#endif /* FIFO */
}

/*
 * Make a hard file link.
 */
/* ARGSUSED */
int
sys_link(struct proc *p, void *v, register_t *retval)
{
	struct sys_link_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	struct vnode *vp;
	struct nameidata nd;
	int error;
	int flags;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;

	flags = LOCKPARENT;
	if (vp->v_type == VDIR) {
		flags |= STRIPSLASHES;
	}

	NDINIT(&nd, CREATE, flags, UIO_USERSPACE, SCARG(uap, link), p);
	if ((error = namei(&nd)) != 0)
		goto out;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out;
	}
	error = VOP_LINK(nd.ni_dvp, vp, &nd.ni_cnd);
out:
	vrele(vp);
	return (error);
}

/*
 * Make a symbolic link.
 */
/* ARGSUSED */
int
sys_symlink(struct proc *p, void *v, register_t *retval)
{
	struct sys_symlink_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	struct vattr vattr;
	char *path;
	int error;
	struct nameidata nd;

	path = pool_get(&namei_pool, PR_WAITOK);
	error = copyinstr(SCARG(uap, path), path, MAXPATHLEN, NULL);
	if (error)
		goto out;
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, link), p);
	if ((error = namei(&nd)) != 0)
		goto out;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out;
	}
	VATTR_NULL(&vattr);
	vattr.va_mode = ACCESSPERMS &~ p->p_fd->fd_cmask;
	error = VOP_SYMLINK(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr, path);
out:
	pool_put(&namei_pool, path);
	return (error);
}

/*
 * Delete a name from the filesystem.
 */
/* ARGSUSED */
int
sys_unlink(struct proc *p, void *v, register_t *retval)
{
	struct sys_unlink_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;

	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_flag & VROOT) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		error = EBUSY;
		goto out;
	}

	(void)uvm_vnp_uncache(vp);

	error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
out:
	return (error);
}

/*
 * Reposition read/write file offset.
 */
int
sys_lseek(struct proc *p, void *v, register_t *retval)
{
	struct sys_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct ucred *cred = p->p_ucred;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vattr vattr;
	struct vnode *vp;
	off_t offarg, newoff;
	int error, special;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return (ESPIPE);
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type == VFIFO)
		return (ESPIPE);
	if (vp->v_type == VCHR)
		special = 1;
	else
		special = 0;
	offarg = SCARG(uap, offset);

	switch (SCARG(uap, whence)) {
	case SEEK_CUR:
		newoff = fp->f_offset + offarg;;
		break;
	case SEEK_END:
		error = VOP_GETATTR((struct vnode *)fp->f_data, &vattr,
				    cred, p);
		if (error)
			return (error);
		newoff = offarg + (off_t)vattr.va_size;
		break;
	case SEEK_SET:
		newoff = offarg;
		break;
	default:
		return (EINVAL);
	}
	if (!special) {
		if (newoff < 0)
			return (EINVAL);
	} else {
		/*
		 * Make sure the user don't seek beyond the end of the
		 * partition.
		 */
		struct partinfo dpart;
		error = vn_ioctl(fp, DIOCGPART, (void *)&dpart, p);
		if (!error) {
			if (newoff >= DL_GETPSIZE(dpart.part) *
			    dpart.disklab->d_secsize)
					return (EINVAL);
		}
	}
	*(off_t *)retval = fp->f_offset = newoff;
	fp->f_seek++;
	return (0);
}

/*
 * Check access permissions.
 */
int
sys_access(struct proc *p, void *v, register_t *retval)
{
	struct sys_access_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct ucred *cred = p->p_ucred;
	struct vnode *vp;
	int error, flags, t_gid, t_uid;
	struct nameidata nd;

	if (SCARG(uap, flags) & ~(R_OK | W_OK | X_OK))
		return (EINVAL);
	t_uid = cred->cr_uid;
	t_gid = cred->cr_gid;
	cred->cr_uid = p->p_cred->p_ruid;
	cred->cr_gid = p->p_cred->p_rgid;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		goto out1;
	vp = nd.ni_vp;

	/* Flags == 0 means only check for existence. */
	if (SCARG(uap, flags)) {
		flags = 0;
		if (SCARG(uap, flags) & R_OK)
			flags |= VREAD;
		if (SCARG(uap, flags) & W_OK)
			flags |= VWRITE;
		if (SCARG(uap, flags) & X_OK)
			flags |= VEXEC;
		if ((flags & VWRITE) == 0 || (error = vn_writechk(vp)) == 0)
			error = VOP_ACCESS(vp, flags, cred, p);
	}
	vput(vp);
out1:
	cred->cr_uid = t_uid;
	cred->cr_gid = t_gid;
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
sys_stat(struct proc *p, void *v, register_t *retval)
{
	struct sys_stat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct stat sb;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	/* Don't let non-root see generation numbers (for NFS security) */
	if (suser(p, 0))
		sb.st_gen = 0;
	error = copyout(&sb, SCARG(uap, ub), sizeof(sb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
sys_lstat(struct proc *p, void *v, register_t *retval)
{
	struct sys_lstat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct stat sb;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	/* Don't let non-root see generation numbers (for NFS security) */
	if (suser(p, 0))
		sb.st_gen = 0;
	error = copyout(&sb, SCARG(uap, ub), sizeof(sb));
	return (error);
}

/*
 * Get configurable pathname variables.
 */
/* ARGSUSED */
int
sys_pathconf(struct proc *p, void *v, register_t *retval)
{
	struct sys_pathconf_args /* {
		syscallarg(const char *) path;
		syscallarg(int) name;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = VOP_PATHCONF(nd.ni_vp, SCARG(uap, name), retval);
	vput(nd.ni_vp);
	return (error);
}

/*
 * Return target name of a symbolic link.
 */
/* ARGSUSED */
int
sys_readlink(struct proc *p, void *v, register_t *retval)
{
	struct sys_readlink_args /* {
		syscallarg(const char *) path;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */ *uap = v;
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VLNK)
		error = EINVAL;
	else {
		aiov.iov_base = SCARG(uap, buf);
		aiov.iov_len = SCARG(uap, count);
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auio.uio_procp = p;
		auio.uio_resid = SCARG(uap, count);
		error = VOP_READLINK(vp, &auio, p->p_ucred);
	}
	vput(vp);
	*retval = SCARG(uap, count) - auio.uio_resid;
	return (error);
}

/*
 * Change flags of a file given a path name.
 */
/* ARGSUSED */
int
sys_chflags(struct proc *p, void *v, register_t *retval)
{
	struct sys_chflags_args /* {
		syscallarg(const char *) path;
		syscallarg(u_int) flags;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else if (SCARG(uap, flags) == VNOVAL)
		error = EINVAL;
	else {
		if (suser(p, 0)) {
			if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
				goto out;
			if (vattr.va_type == VCHR || vattr.va_type == VBLK) {
				error = EINVAL;
				goto out;
			}
		}
		VATTR_NULL(&vattr);
		vattr.va_flags = SCARG(uap, flags);
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
out:
	vput(vp);
	return (error);
}

/*
 * Change flags of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchflags(struct proc *p, void *v, register_t *retval)
{
	struct sys_fchflags_args /* {
		syscallarg(int) fd;
		syscallarg(u_int) flags;
	} */ *uap = v;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount && vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else if (SCARG(uap, flags) == VNOVAL)
		error = EINVAL;
	else {
		if (suser(p, 0)) {
			if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p))
			    != 0)
				goto out;
			if (vattr.va_type == VCHR || vattr.va_type == VBLK) {
				error = EINVAL;
				goto out;
			}
		}
		VATTR_NULL(&vattr);
		vattr.va_flags = SCARG(uap, flags);
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
out:
	VOP_UNLOCK(vp, 0, p);
	FRELE(fp);
	return (error);
}

/*
 * Change mode of a file given path name.
 */
/* ARGSUSED */
int
sys_chmod(struct proc *p, void *v, register_t *retval)
{
	struct sys_chmod_args /* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	if (SCARG(uap, mode) & ~(S_IFMT | ALLPERMS))
		return (EINVAL);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		VATTR_NULL(&vattr);
		vattr.va_mode = SCARG(uap, mode) & ALLPERMS;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	vput(vp);
	return (error);
}

/*
 * Change mode of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchmod(struct proc *p, void *v, register_t *retval)
{
	struct sys_fchmod_args /* {
		syscallarg(int) fd;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	if (SCARG(uap, mode) & ~(S_IFMT | ALLPERMS))
		return (EINVAL);

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount && vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		VATTR_NULL(&vattr);
		vattr.va_mode = SCARG(uap, mode) & ALLPERMS;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	VOP_UNLOCK(vp, 0, p);
	FRELE(fp);
	return (error);
}

/*
 * Set ownership given a path name.
 */
/* ARGSUSED */
int
sys_chown(struct proc *p, void *v, register_t *retval)
{
	struct sys_chown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;
	mode_t mode;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		if ((SCARG(uap, uid) != -1 || SCARG(uap, gid) != -1) &&
		    (suser(p, 0) || suid_clear)) {
			error = VOP_GETATTR(vp, &vattr, p->p_ucred, p);
			if (error)
				goto out;
			mode = vattr.va_mode & ~(VSUID | VSGID);
			if (mode == vattr.va_mode)
				mode = VNOVAL;
		}
		else
			mode = VNOVAL;
		VATTR_NULL(&vattr);
		vattr.va_uid = SCARG(uap, uid);
		vattr.va_gid = SCARG(uap, gid);
		vattr.va_mode = mode;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
out:
	vput(vp);
	return (error);
}

/*
 * Set ownership given a path name, without following links.
 */
/* ARGSUSED */
int
sys_lchown(struct proc *p, void *v, register_t *retval)
{
	struct sys_lchown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;
	mode_t mode;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		if ((SCARG(uap, uid) != -1 || SCARG(uap, gid) != -1) &&
		    (suser(p, 0) || suid_clear)) {
			error = VOP_GETATTR(vp, &vattr, p->p_ucred, p);
			if (error)
				goto out;
			mode = vattr.va_mode & ~(VSUID | VSGID);
			if (mode == vattr.va_mode)
				mode = VNOVAL;
		}
		else
			mode = VNOVAL;
		VATTR_NULL(&vattr);
		vattr.va_uid = SCARG(uap, uid);
		vattr.va_gid = SCARG(uap, gid);
		vattr.va_mode = mode;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
out:
	vput(vp);
	return (error);
}

/*
 * Set ownership given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchown(struct proc *p, void *v, register_t *retval)
{
	struct sys_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct file *fp;
	mode_t mode;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		if ((SCARG(uap, uid) != -1 || SCARG(uap, gid) != -1) &&
		    (suser(p, 0) || suid_clear)) {
			error = VOP_GETATTR(vp, &vattr, p->p_ucred, p);
			if (error)
				goto out;
			mode = vattr.va_mode & ~(VSUID | VSGID);
			if (mode == vattr.va_mode)
				mode = VNOVAL;
		} else
			mode = VNOVAL;
		VATTR_NULL(&vattr);
		vattr.va_uid = SCARG(uap, uid);
		vattr.va_gid = SCARG(uap, gid);
		vattr.va_mode = mode;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
out:
	VOP_UNLOCK(vp, 0, p);
	FRELE(fp);
	return (error);
}

/*
 * Set the access and modification times given a path name.
 */
/* ARGSUSED */
int
sys_utimes(struct proc *p, void *v, register_t *retval)
{
	struct sys_utimes_args /* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */ *uap = v;
	struct vnode *vp;
	struct timeval tv[2];
	struct vattr vattr;
	int error;
	struct nameidata nd;

	VATTR_NULL(&vattr);
	if (SCARG(uap, tptr) == NULL) {
		getmicrotime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin(SCARG(uap, tptr), tv,
		    sizeof(tv));
		if (error)
			return (error);
		/* XXX workaround timeval matching the VFS constant VNOVAL */
		if (tv[0].tv_sec == VNOVAL)
			tv[0].tv_sec = VNOVAL - 1;
		if (tv[1].tv_sec == VNOVAL)
			tv[1].tv_sec = VNOVAL - 1;
	}
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		vattr.va_atime.tv_sec = tv[0].tv_sec;
		vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
		vattr.va_mtime.tv_sec = tv[1].tv_sec;
		vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	vput(vp);
	return (error);
}

/*
 * Set the access and modification times given a file descriptor.
 */
/* ARGSUSED */
int
sys_futimes(struct proc *p, void *v, register_t *retval)
{
	struct sys_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const struct timeval *) tptr;
	} */ *uap = v;
	struct vnode *vp;
	struct timeval tv[2];
	struct vattr vattr;
	int error;
	struct file *fp;

	VATTR_NULL(&vattr);
	if (SCARG(uap, tptr) == NULL) {
		getmicrotime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin(SCARG(uap, tptr), tv,
		    sizeof(tv));
		if (error)
			return (error);
		/* XXX workaround timeval matching the VFS constant VNOVAL */
		if (tv[0].tv_sec == VNOVAL)
			tv[0].tv_sec = VNOVAL - 1;
		if (tv[1].tv_sec == VNOVAL)
			tv[1].tv_sec = VNOVAL - 1;
	}
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_mount && vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else {
		vattr.va_atime.tv_sec = tv[0].tv_sec;
		vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
		vattr.va_mtime.tv_sec = tv[1].tv_sec;
		vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	VOP_UNLOCK(vp, 0, p);
	FRELE(fp);
	return (error);
}

/*
 * Truncate a file given its path name.
 */
/* ARGSUSED */
int
sys_truncate(struct proc *p, void *v, register_t *retval)
{
	struct sys_truncate_args /* {
		syscallarg(const char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0 &&
	    (error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	vput(vp);
	return (error);
}

/*
 * Truncate a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_ftruncate(struct proc *p, void *v, register_t *retval)
{
	struct sys_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	off_t len;
	int error;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	len = SCARG(uap, length);
	if ((fp->f_flag & FWRITE) == 0 || len < 0) {
		error = EINVAL;
		goto bad;
	}
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = len;
		error = VOP_SETATTR(vp, &vattr, fp->f_cred, p);
	}
	VOP_UNLOCK(vp, 0, p);
bad:
	FRELE(fp);
	return (error);
}

/*
 * Sync an open file.
 */
/* ARGSUSED */
int
sys_fsync(struct proc *p, void *v, register_t *retval)
{
	struct sys_fsync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct vnode *vp;
	struct file *fp;
	int error;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	error = VOP_FSYNC(vp, fp->f_cred, MNT_WAIT, p);
#ifdef FFS_SOFTUPDATES
	if (error == 0 && vp->v_mount && (vp->v_mount->mnt_flag & MNT_SOFTDEP))
		error = softdep_fsync(vp);
#endif

	VOP_UNLOCK(vp, 0, p);
	FRELE(fp);
	return (error);
}

/*
 * Rename files.  Source and destination must either both be directories,
 * or both not be directories.  If target is a directory, it must be empty.
 */
/* ARGSUSED */
int
sys_rename(struct proc *p, void *v, register_t *retval)
{
	struct sys_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;
	struct vnode *tvp, *fvp, *tdvp;
	struct nameidata fromnd, tond;
	int error;
	int flags;

	NDINIT(&fromnd, DELETE, WANTPARENT | SAVESTART, UIO_USERSPACE,
	    SCARG(uap, from), p);
	if ((error = namei(&fromnd)) != 0)
		return (error);
	fvp = fromnd.ni_vp;

	flags = LOCKPARENT | LOCKLEAF | NOCACHE | SAVESTART;
	/*
	 * rename("foo/", "bar/");  is  OK
	 */
	if (fvp->v_type == VDIR)
		flags |= STRIPSLASHES;

	NDINIT(&tond, RENAME, flags,
	    UIO_USERSPACE, SCARG(uap, to), p);
	if ((error = namei(&tond)) != 0) {
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
		goto out1;
	}
	tdvp = tond.ni_dvp;
	tvp = tond.ni_vp;
	if (tvp != NULL) {
		if (fvp->v_type == VDIR && tvp->v_type != VDIR) {
			error = ENOTDIR;
			goto out;
		} else if (fvp->v_type != VDIR && tvp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
	}
	if (fvp == tdvp)
		error = EINVAL;
	/*
	 * If source is the same as the destination (that is the
	 * same inode number)
	 */
	if (fvp == tvp)
		error = -1;
out:
	if (!error) {
		if (tvp) {
			(void)uvm_vnp_uncache(tvp);
		}
		error = VOP_RENAME(fromnd.ni_dvp, fromnd.ni_vp, &fromnd.ni_cnd,
				   tond.ni_dvp, tond.ni_vp, &tond.ni_cnd);
	} else {
		VOP_ABORTOP(tond.ni_dvp, &tond.ni_cnd);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
	}
	vrele(tond.ni_startdir);
	pool_put(&namei_pool, tond.ni_cnd.cn_pnbuf);
out1:
	if (fromnd.ni_startdir)
		vrele(fromnd.ni_startdir);
	pool_put(&namei_pool, fromnd.ni_cnd.cn_pnbuf);
	if (error == -1)
		return (0);
	return (error);
}

/*
 * Make a directory file.
 */
/* ARGSUSED */
int
sys_mkdir(struct proc *p, void *v, register_t *retval)
{
	struct sys_mkdir_args /* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, CREATE, LOCKPARENT | STRIPSLASHES,
	    UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(vp);
		return (EEXIST);
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (SCARG(uap, mode) & ACCESSPERMS) &~ p->p_fd->fd_cmask;
	error = VOP_MKDIR(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (!error)
		vput(nd.ni_vp);
	return (error);
}

/*
 * Remove a directory file.
 */
/* ARGSUSED */
int
sys_rmdir(struct proc *p, void *v, register_t *retval)
{
	struct sys_rmdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * No rmdir "." please.
	 */
	if (nd.ni_dvp == vp) {
		error = EBUSY;
		goto out;
	}
	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_flag & VROOT)
		error = EBUSY;
out:
	if (!error) {
		error = VOP_RMDIR(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
	}
	return (error);
}

/*
 * Read a block of directory entries in a file system independent format.
 */
int
sys_getdirentries(struct proc *p, void *v, register_t *retval)
{
	struct sys_getdirentries_args /* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(int) count;
		syscallarg(long *) basep;
	} */ *uap = v;
	struct vnode *vp;
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	long loff;
	int error, eofflag;

	if (SCARG(uap, count) < 0)
		return EINVAL;
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto bad;
	}
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto bad;
	}
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, count);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	auio.uio_resid = SCARG(uap, count);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	loff = auio.uio_offset = fp->f_offset;
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, 0, 0);
	fp->f_offset = auio.uio_offset;
	VOP_UNLOCK(vp, 0, p);
	if (error)
		goto bad;
	error = copyout(&loff, SCARG(uap, basep),
	    sizeof(long));
	*retval = SCARG(uap, count) - auio.uio_resid;
bad:
	FRELE(fp);
	return (error);
}

/*
 * Set the mode mask for creation of filesystem nodes.
 */
int
sys_umask(struct proc *p, void *v, register_t *retval)
{
	struct sys_umask_args /* {
		syscallarg(mode_t) newmask;
	} */ *uap = v;
	struct filedesc *fdp;

	fdp = p->p_fd;
	*retval = fdp->fd_cmask;
	fdp->fd_cmask = SCARG(uap, newmask) & ACCESSPERMS;
	return (0);
}

/*
 * Void all references to file by ripping underlying filesystem
 * away from vnode.
 */
/* ARGSUSED */
int
sys_revoke(struct proc *p, void *v, register_t *retval)
{
	struct sys_revoke_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
		goto out;
	if (p->p_ucred->cr_uid != vattr.va_uid &&
	    (error = suser(p, 0)))
		goto out;
	if (vp->v_usecount > 1 || (vp->v_flag & (VALIASED)))
		VOP_REVOKE(vp, REVOKEALL);
out:
	vrele(vp);
	return (error);
}

/*
 * Convert a user file descriptor to a kernel file entry.
 *
 * On return *fpp is FREF:ed.
 */
int
getvnode(struct filedesc *fdp, int fd, struct file **fpp)
{
	struct file *fp;
	struct vnode *vp;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type == VBAD)
		return (EBADF);

	FREF(fp);
	*fpp = fp;

	return (0);
}

/*
 * Positional read system call.
 */
int
sys_pread(struct proc *p, void *v, register_t *retval)
{
	struct sys_pread_args /* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		return (ESPIPE);
	}

	offset = SCARG(uap, offset);

	FREF(fp);

	/* dofileread() will FRELE the descriptor for us */
	return (dofileread(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, retval));
}

/*
 * Positional scatter read system call.
 */
int
sys_preadv(struct proc *p, void *v, register_t *retval)
{
	struct sys_preadv_args /* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		return (ESPIPE);
	}

	FREF(fp);

	offset = SCARG(uap, offset);

	/* dofilereadv() will FRELE the descriptor for us */
	return (dofilereadv(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, retval));
}

/*
 * Positional write system call.
 */
int
sys_pwrite(struct proc *p, void *v, register_t *retval)
{
	struct sys_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(const void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & FWRITE) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		return (ESPIPE);
	}

	FREF(fp);

	offset = SCARG(uap, offset);

	/* dofilewrite() will FRELE the descriptor for us */
	return (dofilewrite(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, retval));
}

/*
 * Positional gather write system call.
 */
int
sys_pwritev(struct proc *p, void *v, register_t *retval)
{
	struct sys_pwritev_args /* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & FWRITE) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		return (ESPIPE);
	}

	FREF(fp);

	offset = SCARG(uap, offset);

	/* dofilewritev() will FRELE the descriptor for us */
	return (dofilewritev(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, retval));
}
