/*	$NetBSD: tmpfs_fifoops.c,v 1.9 2011/05/24 20:17:49 rmind Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * Copyright (c) 2013 Pedro Martelletto
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
 * tmpfs vnode interface for named pipes.
 */

#if 0
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_fifoops.c,v 1.9 2011/05/24 20:17:49 rmind Exp $");
#endif

#include <sys/param.h>
#include <sys/vnode.h>

#include <tmpfs/tmpfs.h>
#include <tmpfs/tmpfs_fifoops.h>

/*
 * vnode operations vector used for fifos stored in a tmpfs file system.
 */

struct vops tmpfs_fifovops = {
	.vop_lookup	= tmpfs_fifo_lookup,
	.vop_create	= tmpfs_fifo_create,
	.vop_mknod	= tmpfs_fifo_mknod,
	.vop_open	= tmpfs_fifo_open,
	.vop_close	= tmpfs_fifo_close,
	.vop_access	= tmpfs_fifo_access,
	.vop_getattr	= tmpfs_fifo_getattr,
	.vop_setattr	= tmpfs_fifo_setattr,
	.vop_read	= tmpfs_fifo_read,
	.vop_write	= tmpfs_fifo_write,
	.vop_ioctl	= tmpfs_fifo_ioctl,
	.vop_poll	= tmpfs_fifo_poll,
	.vop_kqfilter	= tmpfs_fifo_kqfilter,
	.vop_revoke	= tmpfs_fifo_revoke,
	.vop_fsync	= tmpfs_fifo_fsync,
	.vop_remove	= tmpfs_fifo_remove,
	.vop_link	= tmpfs_fifo_link,
	.vop_rename	= tmpfs_fifo_rename,
	.vop_mkdir	= tmpfs_fifo_mkdir,
	.vop_rmdir	= tmpfs_fifo_rmdir,
	.vop_symlink	= tmpfs_fifo_symlink,
	.vop_readdir	= tmpfs_fifo_readdir,
	.vop_readlink	= tmpfs_fifo_readlink,
	.vop_abortop	= tmpfs_fifo_abortop,
	.vop_inactive	= tmpfs_fifo_inactive,
	.vop_reclaim	= tmpfs_fifo_reclaim,
	.vop_lock	= tmpfs_fifo_lock,
	.vop_unlock	= tmpfs_fifo_unlock,
	.vop_bmap	= tmpfs_fifo_bmap,
	.vop_strategy	= tmpfs_fifo_strategy,
	.vop_print	= tmpfs_fifo_print,
	.vop_islocked	= tmpfs_fifo_islocked,
	.vop_pathconf	= tmpfs_fifo_pathconf,
	.vop_advlock	= tmpfs_fifo_advlock,
	.vop_bwrite	= tmpfs_fifo_bwrite,
};

int
tmpfs_fifo_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	tmpfs_update(vp, NULL, NULL, 0);
	return (fifo_close(v));
}

int
tmpfs_fifo_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VP_TO_TMPFS_NODE(vp)->tn_status |= TMPFS_NODE_ACCESSED;
	return (fifo_read(v));
}

int
tmpfs_fifo_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VP_TO_TMPFS_NODE(vp)->tn_status |= TMPFS_NODE_MODIFIED;
	return (fifo_write(v));
}

int
tmpfs_fifo_fsync(void *v)
{
	return (0);
}
