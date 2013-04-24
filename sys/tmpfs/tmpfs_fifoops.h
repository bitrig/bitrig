/*	$NetBSD: tmpfs_fifoops.h,v 1.8 2011/05/24 20:17:49 rmind Exp $	*/

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

#ifndef _FS_TMPFS_TMPFS_FIFOOPS_H_
#define _FS_TMPFS_TMPFS_FIFOOPS_H_

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#include <miscfs/fifofs/fifo.h>
#include <tmpfs/tmpfs_vnops.h>

/*
 * Declarations for tmpfs_fifoops.c.
 */

extern struct vops tmpfs_fifovops;

#define	tmpfs_fifo_lookup	vop_generic_lookup
#define	tmpfs_fifo_create	fifo_badop
#define	tmpfs_fifo_mknod	fifo_badop
#define	tmpfs_fifo_open		fifo_open
int	tmpfs_fifo_close	(void *);
#define	tmpfs_fifo_access	tmpfs_access
#define	tmpfs_fifo_getattr	tmpfs_getattr
#define	tmpfs_fifo_setattr	tmpfs_setattr
int	tmpfs_fifo_read		(void *);
int	tmpfs_fifo_write	(void *);
#define	tmpfs_fifo_ioctl	fifo_ioctl
#define	tmpfs_fifo_poll		fifo_poll
#define	tmpfs_fifo_kqfilter	fifo_kqfilter
#define	tmpfs_fifo_revoke	vop_generic_revoke
int	tmpfs_fifo_fsync	(void *);
#define	tmpfs_fifo_remove	fifo_badop
#define	tmpfs_fifo_link		fifo_badop
#define	tmpfs_fifo_rename	fifo_badop
#define	tmpfs_fifo_mkdir	fifo_badop
#define	tmpfs_fifo_rmdir	fifo_badop
#define	tmpfs_fifo_symlink	fifo_badop
#define	tmpfs_fifo_readdir	fifo_badop
#define	tmpfs_fifo_readlink	fifo_badop
#define	tmpfs_fifo_abortop	fifo_badop
#define	tmpfs_fifo_inactive	tmpfs_inactive
#define	tmpfs_fifo_reclaim	tmpfs_reclaim
#define	tmpfs_fifo_lock		tmpfs_lock
#define	tmpfs_fifo_unlock	tmpfs_unlock
#define	tmpfs_fifo_bmap		vop_generic_bmap
#define	tmpfs_fifo_strategy	fifo_badop
#define	tmpfs_fifo_print	tmpfs_print
#define	tmpfs_fifo_pathconf	fifo_pathconf
#define	tmpfs_fifo_islocked	tmpfs_islocked
#define	tmpfs_fifo_advlock	fifo_advlock
#define	tmpfs_fifo_bwrite	tmpfs_bwrite

#endif /* _FS_TMPFS_TMPFS_FIFOOPS_H_ */
