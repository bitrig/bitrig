/*	$NetBSD: tmpfs_specops.c,v 1.10 2011/05/24 20:17:49 rmind Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
 * tmpfs vnode interface for special devices.
 */

#if 0
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_specops.c,v 1.10 2011/05/24 20:17:49 rmind Exp $");
#endif

#include <sys/param.h>
#include <sys/vnode.h>

#include <tmpfs/tmpfs.h>
#include <tmpfs/tmpfs_specops.h>

/*
 * vnode operations vector used for special devices stored in a tmpfs
 * file system.
 */

struct vops tmpfs_specvops = {
	.vop_lookup	= tmpfs_spec_lookup,
	.vop_create	= tmpfs_spec_create,
	.vop_mknod	= tmpfs_spec_mknod,
	.vop_open	= tmpfs_spec_open,
	.vop_close	= tmpfs_spec_close,
	.vop_access	= tmpfs_spec_access,
	.vop_getattr	= tmpfs_spec_getattr,
	.vop_setattr	= tmpfs_spec_setattr,
	.vop_read	= tmpfs_spec_read,
	.vop_write	= tmpfs_spec_write,
	.vop_ioctl	= tmpfs_spec_ioctl,
	.vop_poll	= tmpfs_spec_poll,
	.vop_kqfilter	= tmpfs_spec_kqfilter,
	.vop_revoke	= tmpfs_spec_revoke,
	.vop_fsync	= tmpfs_spec_fsync,
	.vop_remove	= tmpfs_spec_remove,
	.vop_link	= tmpfs_spec_link,
	.vop_rename	= tmpfs_spec_rename,
	.vop_mkdir	= tmpfs_spec_mkdir,
	.vop_rmdir	= tmpfs_spec_rmdir,
	.vop_symlink	= tmpfs_spec_symlink,
	.vop_readdir	= tmpfs_spec_readdir,
	.vop_readlink	= tmpfs_spec_readlink,
	.vop_abortop	= tmpfs_spec_abortop,
	.vop_inactive	= tmpfs_spec_inactive,
	.vop_reclaim	= tmpfs_spec_reclaim,
	.vop_lock	= tmpfs_spec_lock,
	.vop_unlock	= tmpfs_spec_unlock,
	.vop_bmap	= tmpfs_spec_bmap,
	.vop_strategy	= tmpfs_spec_strategy,
	.vop_print	= tmpfs_spec_print,
	.vop_islocked	= tmpfs_spec_islocked,
	.vop_pathconf	= tmpfs_spec_pathconf,
	.vop_advlock	= tmpfs_spec_advlock,
	.vop_bwrite	= tmpfs_spec_bwrite,
};

int
tmpfs_spec_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	tmpfs_update(vp, NULL, NULL, 0);
	return (spec_close(ap));
}

int
tmpfs_spec_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VP_TO_TMPFS_NODE(vp)->tn_status |= TMPFS_NODE_ACCESSED;
	return (spec_read(ap));
}

int
tmpfs_spec_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VP_TO_TMPFS_NODE(vp)->tn_status |= TMPFS_NODE_MODIFIED;
	return (spec_write(ap));
}
