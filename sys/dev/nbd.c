/*
 * Copyright (c) 2015 Patrick Wildt <patrick@blueri.se>
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
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/dkio.h>
#include <sys/specdev.h>
#include <sys/kthread.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#ifdef NBDDEBUG
int nbddebug = 0x00;
#define	NDB_FOLLOW	0x01
#define	NDB_INIT	0x02
#define	NDB_IO		0x04
#define	DNPRINTF(f, p...)	do { if ((f) & nbddebug) printf(p); } while (0)
#else
#define	DNPRINTF(f, p...)	/* nothing */
#endif	/* NBDDEBUG */

/* NBD IOCTL API */
#define NBD_SET_SOCK		_IO( 0xab, 0 )
#define NBD_SET_BLKSIZE		_IO( 0xab, 1 )
#define NBD_SET_SIZE		_IO( 0xab, 2 )
#define NBD_DO_IT		_IO( 0xab, 3 )
#define NBD_CLEAR_SOCK		_IO( 0xab, 4 )
#define NBD_CLEAR_QUE		_IO( 0xab, 5 )
#define NBD_PRINT_DEBUG		_IO( 0xab, 6 )
#define NBD_SET_SIZE_BLOCKS	_IO( 0xab, 7 )
#define NBD_DISCONNECT		_IO( 0xab, 8 )
#define NBD_SET_TIMEOUT		_IO( 0xab, 9 )
#define NBD_SET_FLAGS		_IO( 0xab, 10)

struct nbd_request {
	uint32_t	 magic;
#define NBD_REQUEST_MAGIC 0x25609513
	uint32_t	 type;
#define NBD_CMD_READ	0
#define NBD_CMD_WRITE	1
#define NBD_CMD_DISC	2
#define NBD_CMD_FLUSH	3
#define NBD_CMD_TRIM	4
	uint64_t	 handle;
	uint64_t	 from;			/* offset in bytes */
	uint32_t	 len;			/* length in bytes */
} __attribute__((packed));

struct nbd_reply {
	uint32_t	 magic;
#define NBD_REPLY_MAGIC	0x67446698
	uint32_t	 error;			/* 0 == OK */
	uint64_t	 handle;		/* identify request */
} __attribute__((packed));

struct nbd_wait {
	struct buf	*bp;			/* Pointer to read/write buf. */
	int		 error;			/* 0 == OK */
	uint32_t	 cmd;			/* used for READ/WRITE */
	uint64_t	 from;			/* offset in bytes */
	TAILQ_ENTRY(nbd_wait) next;		/* next in list */
};

struct nbd_softc {
	struct device	 sc_dev;
	struct disk	 sc_dk;

	int		 sc_flags;		/* flags */
#define	NBF_INITED	0x0001
#define	NBF_HAVELABEL	0x0002
#define	NBF_READONLY	0x0004
	size_t		 sc_nsectors;		/* size of nbd in sectors */
	size_t		 sc_secsize;		/* sector size in bytes */

	struct file	*sc_socket;		/* socket */
	int		 sc_timeout;		/* transfer timeout */
	int		 sc_disconnect;		/* user requests disconnect */
	struct proc	*sc_kthread;		/* kthread proc */

	TAILQ_HEAD(, nbd_wait) sc_sendq;	/* requests to be sent */
	TAILQ_HEAD(, nbd_wait) sc_waitq;	/* requests that need a reply */
};

struct nbd_softc *nbd_softc;
int numnbd = 0;

/* prototypes */
int		 nbdgetdisklabel(dev_t, struct nbd_softc *, struct disklabel *, int);
int		 nbd_do_it(struct nbd_softc *);
void		 nbd_send_thread(void *);
struct nbd_wait *nbd_handle_to_req(struct nbd_softc *, struct nbd_reply *);
void		 nbd_clear_queues(struct nbd_softc *);
int		 nbd_transfer(struct nbd_softc *, int, void *, size_t);

void
nbdattach(int num)
{
	char *mem;
	int i;

	if (num <= 0)
		return;
	mem = mallocarray(num, sizeof(struct nbd_softc), M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (mem == NULL) {
		printf("WARNING: no memory for network block devices\n");
		return;
	}
	nbd_softc = (struct nbd_softc *)mem;
	for (i = 0; i < num; i++) {
		struct nbd_softc *sc = &nbd_softc[i];

		sc->sc_dev.dv_unit = i;
		snprintf(sc->sc_dev.dv_xname, sizeof(sc->sc_dev.dv_xname),
		    "nbd%d", i);
		disk_construct(&sc->sc_dk);
		device_ref(&sc->sc_dev);

		TAILQ_INIT(&sc->sc_sendq);
		TAILQ_INIT(&sc->sc_waitq);
	}
	numnbd = num;
}

int
nbdopen(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = DISKUNIT(dev);
	struct nbd_softc *sc;
	int error = 0, part;

	DNPRINTF(NDB_FOLLOW, "nbdopen(%x, %x, %x, %p)\n", dev, flags, mode, p);

	if (unit >= numnbd)
		return (ENXIO);
	sc = &nbd_softc[unit];

	if ((error = disk_lock(&sc->sc_dk)) != 0)
		return (error);

	if ((flags & FWRITE) && (sc->sc_flags & NBF_READONLY)) {
		error = EROFS;
		goto bad;
	}

	if ((sc->sc_flags & NBF_INITED) &&
	    (sc->sc_flags & NBF_HAVELABEL) == 0) {
		sc->sc_flags |= NBF_HAVELABEL;
		nbdgetdisklabel(dev, sc, sc->sc_dk.dk_label, 0);
	}

	part = DISKPART(dev);
	error = disk_openpart(&sc->sc_dk, part, mode,
	    (sc->sc_flags & NBF_HAVELABEL) != 0);

bad:
	disk_unlock(&sc->sc_dk);
	return (error);
}

/*
 * Load the label information on the named device
 */
int
nbdgetdisklabel(dev_t dev, struct nbd_softc *sc, struct disklabel *lp,
    int spoofonly)
{
	memset(lp, 0, sizeof(struct disklabel));

	/* Fake most of it. */
	lp->d_type = DTYPE_SCSI;
	lp->d_ntracks = 1;
	lp->d_ncylinders = 1;

	strncpy(lp->d_typename, "nbd device", sizeof(lp->d_typename));
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));

	/* Derive the file system geometry from provided values. */
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_nsectors = sc->sc_nsectors;
	lp->d_secsize = sc->sc_secsize;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	DL_SETDSIZE(lp, lp->d_nsectors);

	lp->d_flags = 0;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);

	/* Call the generic disklabel extraction routine */
	return readdisklabel(DISKLABELDEV(dev), nbdstrategy, lp, spoofonly);
}

int
nbdclose(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = DISKUNIT(dev);
	struct nbd_softc *sc;
	int part;

	DNPRINTF(NDB_FOLLOW, "nbdclose(%x, %x, %x, %p)\n", dev, flags, mode, p);

	if (unit >= numnbd)
		return (ENXIO);
	sc = &nbd_softc[unit];

	disk_lock_nointr(&sc->sc_dk);

	part = DISKPART(dev);

	disk_closepart(&sc->sc_dk, part, mode);

	disk_unlock(&sc->sc_dk);
	return (0);
}

void
nbdstrategy(struct buf *bp)
{
	int unit = DISKUNIT(bp->b_dev);
	struct nbd_softc *sc;
	struct nbd_wait *req;
	struct partition *p;
	off_t off;
	int s;

	DNPRINTF(NDB_FOLLOW, "nbdstrategy(%p): unit %d\n", bp, unit);

	if (unit >= numnbd) {
		bp->b_error = ENXIO;
		goto bad;
	}
	sc = &nbd_softc[unit];

	if ((sc->sc_flags & NBF_HAVELABEL) == 0) {
		bp->b_error = ENXIO;
		goto bad;
	}

	if (bounds_check_with_label(bp, sc->sc_dk.dk_label) == -1) {
		bp->b_resid = bp->b_bcount;
		goto done;
	}

	p = &sc->sc_dk.dk_label->d_partitions[DISKPART(bp->b_dev)];
	off = DL_GETPOFFSET(p) * sc->sc_dk.dk_label->d_secsize +
	    DL_BLKTOSEC(sc->sc_dk.dk_label, bp->b_blkno) *
	    sc->sc_dk.dk_label->d_secsize;

	/* Insert bp/request into queue. */
	req = malloc(sizeof(struct nbd_wait), M_DEVBUF, M_WAITOK | M_ZERO);
	if (req == NULL)
		goto bad;

	req->bp = bp;
	req->from = off;
	req->cmd = (bp->b_flags & B_READ) ? NBD_CMD_READ : NBD_CMD_WRITE;
	TAILQ_INSERT_TAIL(&sc->sc_sendq, req, next);
	wakeup(&sc->sc_sendq);

	/* Wait until done. */
	tsleep(req, PWAIT, sc->sc_dev.dv_xname, 0);
	if (req->error != 0) {
		free(req, M_DEVBUF, 0);
		goto bad;
	}

	/* Data copied by receiving/sending thread. */
	bp->b_resid = 0;
	free(req, M_DEVBUF, 0);

	goto done;

bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
done:
	s = splbio();
	biodone(bp);
	splx(s);
}

/* ARGSUSED */
int
nbdread(dev_t dev, struct uio *uio, int flags)
{
	return (physio(nbdstrategy, dev, B_READ, minphys, uio));
}

/* ARGSUSED */
int
nbdwrite(dev_t dev, struct uio *uio, int flags)
{
	return (physio(nbdstrategy, dev, B_WRITE, minphys, uio));
}

/* ARGSUSED */
int
nbdioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	int unit = DISKUNIT(dev);
	struct disklabel *lp;
	struct nbd_softc *sc;
	struct nbd_request r;
	int arg = *((int *)addr);
	int error;

	DNPRINTF(NDB_FOLLOW, "nbdioctl(%x, %lx, %p, %x, %p): unit %d\n",
	    dev, cmd, addr, flag, p, unit);

	error = suser(p, 0);
	if (error)
		return (error);
	if (unit >= numnbd)
		return (ENXIO);

	sc = &nbd_softc[unit];

	switch (cmd) {
	case NBD_SET_SOCK:
		if (sc->sc_socket != NULL)
			return (EBUSY);
		if ((error = getsock(p->p_fd, arg, &sc->sc_socket)) != 0)
			return (error);
		sc->sc_disconnect = 0;
		break;

	case NBD_SET_BLKSIZE:
		sc->sc_nsectors = ((uint64_t)sc->sc_nsectors * sc->sc_secsize) / arg;
		sc->sc_secsize = arg;
		break;

	case NBD_SET_SIZE:
		sc->sc_nsectors = arg / sc->sc_secsize;
		break;

	case NBD_DO_IT:
		/* Already running? */
		if (ISSET(sc->sc_flags, NBF_INITED))
			return (EBUSY);

		/* Attach disk. */
		sc->sc_dk.dk_name = sc->sc_dev.dv_xname;
		disk_attach(&sc->sc_dev, &sc->sc_dk);

		/* We're inited now. */
		sc->sc_flags |= NBF_INITED;

		/* Setup kthread that sends the packets. */
		kthread_create(nbd_send_thread, sc, &sc->sc_kthread,
		    sc->sc_dev.dv_xname);

		/* Receive all replies and end requests. Blocks. */
		error = nbd_do_it(sc);
		if (sc->sc_disconnect)
			error = 0;

		/* We're gone now. */
		sc->sc_flags = 0;

		/* Stop kthread. */
		sc->sc_disconnect = 1;
		wakeup(&sc->sc_sendq);
		if (sc->sc_kthread)
			tsleep(&sc->sc_kthread, PWAIT, sc->sc_dev.dv_xname, 0);

		/* Send disconnect. */
		memset(&r, 0, sizeof(r));
		r.magic = htobe32(NBD_REQUEST_MAGIC);
		r.type = htobe32(NBD_CMD_DISC);
		nbd_transfer(sc, 1, &r, sizeof(r));

		/* Clear all remaining requests. */
		nbd_clear_queues(sc);

		/* Release socket. */
		if (sc->sc_socket != NULL)
			FRELE(sc->sc_socket, p);
		sc->sc_socket = NULL;

		/* Detach the disk. */
		disk_gone(&sc->sc_dk);
		disk_detach(&sc->sc_dk);

		return (error);

	case NBD_CLEAR_SOCK:
		/* Release socket. */
		if (sc->sc_socket != NULL)
			FRELE(sc->sc_socket, p);
		sc->sc_socket = NULL;

		/* Clear all remaining requests. */
		nbd_clear_queues(sc);

		if ((sc->sc_flags & NBF_INITED) == 0)
			break;

		/* Detach the disk. */
		disk_gone(&sc->sc_dk);
		disk_detach(&sc->sc_dk);
		break;

	case NBD_CLEAR_QUE:
		/* Queue cleared by NBD_DO_IT or NBC_CLEAR_SOCK. */
		break;

	case NBD_PRINT_DEBUG:
		/* TODO: Do debug printfs? */
		break;

	case NBD_SET_SIZE_BLOCKS:
		sc->sc_nsectors = arg;
		break;

	case NBD_DISCONNECT:
		if ((sc->sc_dk.dk_openmask & ~(1 << RAW_PART)) != 0)
			return (EBUSY);

		/* Wakeup waitq, which will wakeup sendq. */
		sc->sc_disconnect = 1;
		wakeup(&sc->sc_waitq);
		if (sc->sc_kthread)
			tsleep(&sc->sc_kthread, PWAIT, sc->sc_dev.dv_xname, 0);
		break;

	case NBD_SET_TIMEOUT:
		/* TODO: Use the timeout. */
		sc->sc_timeout = arg;
		break;

	case NBD_SET_FLAGS:
		/* TODO: Implement flags. */
		break;

	case DIOCRLDINFO:
		if ((sc->sc_flags & NBF_HAVELABEL) == 0)
			return (ENOTTY);
		lp = malloc(sizeof(*lp), M_TEMP, M_WAITOK);
		nbdgetdisklabel(dev, sc, lp, 0);
		*(sc->sc_dk.dk_label) = *lp;
		free(lp, M_TEMP, 0);
		return (0);

	case DIOCGPDINFO:
		if ((sc->sc_flags & NBF_HAVELABEL) == 0)
			return (ENOTTY);
		nbdgetdisklabel(dev, sc, (struct disklabel *)addr, 1);
		return (0);

	case DIOCGDINFO:
		if ((sc->sc_flags & NBF_HAVELABEL) == 0)
			return (ENOTTY);
		*(struct disklabel *)addr = *(sc->sc_dk.dk_label);
		return (0);

	case DIOCGPART:
		if ((sc->sc_flags & NBF_HAVELABEL) == 0)
			return (ENOTTY);
		((struct partinfo *)addr)->disklab = sc->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sc->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		return (0);

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((sc->sc_flags & NBF_HAVELABEL) == 0)
			return (ENOTTY);
		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = disk_lock(&sc->sc_dk)) != 0)
			return (error);

		error = setdisklabel(sc->sc_dk.dk_label,
		    (struct disklabel *)addr, /* sc->sc_dk.dk_openmask */ 0);
		if (error == 0) {
			if (cmd == DIOCWDINFO)
				error = writedisklabel(DISKLABELDEV(dev),
				    nbdstrategy, sc->sc_dk.dk_label);
		}

		disk_unlock(&sc->sc_dk);
		return (error);

	default:
		return (ENOTTY);
	}

	return (0);
}

daddr_t
nbdsize(dev_t dev)
{
	/* We don't support swapping to nbd. */
	return (-1);
}

int
nbddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	/* Not implemented. */
	return (ENXIO);
}

int
nbd_do_it(struct nbd_softc *sc)
{
	int error = 0;
	struct nbd_wait *req;
	struct nbd_reply r;

	while (error == 0) {
		if (sc->sc_disconnect)
			return error;

		if (TAILQ_EMPTY(&sc->sc_waitq)) {
			tsleep(&sc->sc_waitq, PWAIT, sc->sc_dev.dv_xname, 0);
			continue;
		}

		error = nbd_transfer(sc, 0, &r, sizeof(r));
		if (error) {
			printf("%s: transfer error %d\n",
			    sc->sc_dev.dv_xname, error);
			break;
		}

		/* Invalid magic. */
		if (betoh32(r.magic) != NBD_REPLY_MAGIC) {
			printf("%s: invalid magic\n",
			    sc->sc_dev.dv_xname);
			break;
		}

		req = nbd_handle_to_req(sc, &r);
		if (req == NULL) {
			printf("%s: cannot find request\n",
			    sc->sc_dev.dv_xname);
			break;
		}

		req->error = betoh32(r.error);
		if (req->error == 0 && req->cmd == NBD_CMD_READ) {
			error = nbd_transfer(sc, 0, req->bp->b_data,
			    req->bp->b_bcount);
			req->error = error;
		}

		wakeup(req);
	}

	return error;
}

void
nbd_send_thread(void *arg)
{
	struct nbd_softc *sc = (struct nbd_softc *)arg;
	struct nbd_wait *req;
	struct nbd_request r;
	int error = 0;

	while (!sc->sc_disconnect) {
		req = TAILQ_FIRST(&sc->sc_sendq);
		if (req == NULL) {
			tsleep(&sc->sc_sendq, PWAIT, sc->sc_dev.dv_xname, 0);
			continue;
		}

		TAILQ_REMOVE(&sc->sc_sendq, req, next);

		r.magic = htobe32(NBD_REQUEST_MAGIC);
		r.type = htobe32(req->cmd);
		r.handle = htobe64(((uint32_t)req));
		r.from = htobe64(req->from);
		r.len = htobe32(req->bp->b_bcount);

		error = nbd_transfer(sc, 1, &r, sizeof(r));
		if (error) {
			printf("%s: request transfer error %d\n",
			    sc->sc_dev.dv_xname, error);
			break;
		}

		if (req->cmd == NBD_CMD_WRITE) {
			error = nbd_transfer(sc, 1, req->bp->b_data, req->bp->b_bcount);
			if (error) {
				printf("%s: data transfer error %d\n",
				    sc->sc_dev.dv_xname, error);
				break;
			}
		}

		TAILQ_INSERT_TAIL(&sc->sc_waitq, req, next);
		wakeup(&sc->sc_waitq);
	}

	/* Wakeup listeners, we're gone. */
	sc->sc_kthread = NULL;
	wakeup(&sc->sc_kthread);

	kthread_exit(error);
}

struct nbd_wait *
nbd_handle_to_req(struct nbd_softc *sc, struct nbd_reply *reply)
{
	struct nbd_wait *req, *tmp;

	TAILQ_FOREACH_SAFE(req, &sc->sc_waitq, next, tmp) {
		if (betoh64(reply->handle) != (uint32_t)req)
			continue;

		TAILQ_REMOVE(&sc->sc_waitq, req, next);
		return req;
	}

	return NULL;
}

void
nbd_clear_queues(struct nbd_softc *sc)
{
	struct nbd_wait *req, *tmp;

	TAILQ_FOREACH_SAFE(req, &sc->sc_sendq, next, tmp) {
		req->error = 1;
		wakeup(req);
		TAILQ_REMOVE(&sc->sc_sendq, req, next);
	}

	TAILQ_FOREACH_SAFE(req, &sc->sc_waitq, next, tmp) {
		req->error = 1;
		wakeup(req);
		TAILQ_REMOVE(&sc->sc_waitq, req, next);
	}
}

int
nbd_transfer(struct nbd_softc *sc, int send, void *buf, size_t size)
{
	int error;
	struct uio auio;
	struct iovec aio;

	aio.iov_base = buf;
	aio.iov_len = size;
	auio.uio_iov = &aio;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = send ? UIO_WRITE : UIO_READ;
	auio.uio_offset = 0;
	auio.uio_resid = size;
	auio.uio_procp = curproc;
	do {
		if (send)
			error = sosend(sc->sc_socket->f_data, NULL,
			    &auio, NULL, NULL, 0);
		else {
			error = soreceive(sc->sc_socket->f_data, NULL,
				    &auio, NULL, NULL, NULL, 0);
		}
	} while (error == EWOULDBLOCK || error == EINTR ||
	    error == ERESTART || (error == 0 && auio.uio_resid > 0));

	return error;
}
