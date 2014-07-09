/*
 * Copyright (c) 2014 Pedro Martelletto
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
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/dkio.h>
#include <sys/workq.h>

void	tmpfsrdattach(int);
void	tmpfsrd_terminate(void *, void *);
int	tmpfsrd_match(struct device *, void *, void *);
void	tmpfsrd_attach(struct device *, struct device *, void *);
int	tmpfsrd_detach(struct device *, int);

struct tmpfsrd_softc {
	struct device	sc_dev;
	struct disk	sc_dk;
	int		sc_flags;
};

#define TMPFSRD_TERMINATING	0x1

struct cfattach tmpfsrd_ca = {
	sizeof(struct tmpfsrd_softc),
	tmpfsrd_match,
	tmpfsrd_attach,
	tmpfsrd_detach
};

struct cfdriver tmpfsrd_cd = {
	NULL,
	"tmpfsrd",
	DV_DISK
};

extern char	*esym, *eramdisk;
uint8_t		*tmpfsrd_disk;
uint64_t	 tmpfsrd_size;
uint64_t	 tmpfsrd_read;

int	tmpfsrdgetdisklabel(dev_t, struct tmpfsrd_softc *, struct disklabel *, int);

void
tmpfsrdattach(int num)
{
	static struct cfdata cf; /* Fake cf. */
	struct tmpfsrd_softc *sc;

	if (eramdisk == NULL)
		return;

	/* XXX: Fake up more? */
	cf.cf_attach = &tmpfsrd_ca;
	cf.cf_driver = &tmpfsrd_cd;
	cf.cf_fstate = FSTATE_FOUND;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sc == NULL)
		panic("tmpfsrdattach: out of memory");

	strncpy(sc->sc_dev.dv_xname, "tmpfsrd0", sizeof(sc->sc_dev.dv_xname));
	sc->sc_dev.dv_class = DV_DISK;
	sc->sc_dev.dv_cfdata = &cf;
	sc->sc_dev.dv_flags = DVF_ACTIVE;
	sc->sc_dev.dv_unit = 0;
	sc->sc_dev.dv_ref = 1;

	tmpfsrd_cd.cd_devs = malloc(sizeof(void *), M_DEVBUF, M_NOWAIT);
	if (tmpfsrd_cd.cd_devs == NULL)
		panic("tmpfsrdattach: out of memory");

	tmpfsrd_cd.cd_devs[0] = sc;
	tmpfsrd_cd.cd_ndevs = 1;

	tmpfsrd_attach(NULL, &sc->sc_dev, NULL);

	TAILQ_INSERT_TAIL(&alldevs, &sc->sc_dev, dv_list);
	device_ref(&sc->sc_dev);
}

/*
 * We are a pseudo-device faking a disk driver. This function is provided for
 * the sake of completeness and isonomy with regular disk drivers, but it is
 * not called in practise.
 */
int
tmpfsrd_match(struct device *parent, void *match, void *aux)
{
	return (0);
}

/*
 * This function is exported together with tmpfsrd_match(). In practise,
 * however, it is only called through tmpfsrdattach().
 */
void
tmpfsrd_attach(struct device *parent, struct device *self, void *aux)
{
	struct tmpfsrd_softc *sc = (struct tmpfsrd_softc *)self;
	uint64_t mbytes, kbytes;

	tmpfsrd_disk = (uint8_t *)roundup((paddr_t)esym, PAGE_SIZE);
	tmpfsrd_size = (paddr_t)eramdisk - (paddr_t)tmpfsrd_disk;

	KASSERT(eramdisk != NULL && ((vaddr_t)eramdisk & PAGE_MASK) == 0);
	KASSERT((tmpfsrd_size & PAGE_MASK) == 0);
#ifdef __HAVE_PMAP_DIRECT
	KASSERT(!pmap_is_direct_mapped((vaddr_t)tmpfsrd_disk) &&
		!pmap_is_direct_mapped((vaddr_t)tmpfsrd_disk + tmpfsrd_size));
#endif

	sc->sc_dk.dk_name = sc->sc_dev.dv_xname;

	printf("%s: ", sc->sc_dk.dk_name);

	kbytes = tmpfsrd_size / 1024;
	mbytes = kbytes / 1024;

	if (mbytes == 0 && kbytes == 0)
		printf("%lluB ", tmpfsrd_size);
	else if (mbytes == 0)
		printf("%llukB ", kbytes);
	else
		printf("%lluMB ", mbytes);

	printf("ramdisk ready\n");

	disk_attach(&sc->sc_dev, &sc->sc_dk);
}

int
tmpfsrd_detach(struct device *self, int flags)
{
	struct tmpfsrd_softc *sc = (struct tmpfsrd_softc *)self;

	/*
	 * We pass tmpfsrdopen() as a key for the disk subsystem to look us up.
	 */
	disk_gone(tmpfsrdopen, self->dv_unit);
	disk_detach(&sc->sc_dk);

	return (0);
}

int
tmpfsrdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit = DISKUNIT(dev);
	int part = DISKUNIT(dev);
	int error;
	struct tmpfsrd_softc *sc = (void *)disk_lookup(&tmpfsrd_cd, unit);

	if (sc == NULL)
		return (ENXIO);

	if (flag & FWRITE)
		return (EOPNOTSUPP);

	error = disk_lock(&sc->sc_dk);
	if (error)
		goto unref;

	if (sc->sc_dk.dk_openmask == 0) {
		/* Load the partition info if not already loaded. */
		error = tmpfsrdgetdisklabel(dev, sc, sc->sc_dk.dk_label, 0);
		if (error)
			goto unlock;
	}

	error = disk_openpart(&sc->sc_dk, part, fmt, 1);

unlock:
	disk_unlock(&sc->sc_dk);
unref:
	device_unref(&sc->sc_dev);
	return (error);
}

void
tmpfsrd_terminate(void *arg1, void *arg2)
{
	config_detach((struct device *)arg1, 0);
	explicit_bzero(tmpfsrd_disk, tmpfsrd_size);
	pmap_kremove((vaddr_t)tmpfsrd_disk, tmpfsrd_size);
#ifndef VM_PHYSSEG_NOADD
	{
		paddr_t start = (paddr_t)tmpfsrd_disk - KERNBASE;
		paddr_t end = (paddr_t)eramdisk - KERNBASE;
		uvm_page_physload(atop(start), atop(end), atop(start),
		    atop(end), 0);
	}
#endif /* !VM_PHYSSEG_NOADD */
}

int
tmpfsrdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit = DISKUNIT(dev);
	int part = DISKUNIT(dev);
	struct tmpfsrd_softc *sc = (void *)disk_lookup(&tmpfsrd_cd, unit);

	if (sc == NULL)
		return (ENXIO);

	disk_lock_nointr(&sc->sc_dk);
	disk_closepart(&sc->sc_dk, part, fmt);
	disk_unlock(&sc->sc_dk);

	device_unref(&sc->sc_dev);

	if (tmpfsrd_read == tmpfsrd_size &&
	    (sc->sc_flags & TMPFSRD_TERMINATING) == 0) {
		sc->sc_flags |= TMPFSRD_TERMINATING;
		return (workq_add_task(NULL, 0, tmpfsrd_terminate, sc, NULL));
	}

	return (0);
}

void
tmpfsrdstrategy(struct buf *bp)
{
	uint64_t off, len;
	int unit = DISKUNIT(bp->b_dev), s;
	struct tmpfsrd_softc *sc = (void *)disk_lookup(&tmpfsrd_cd, unit);

	if (sc == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}

	if ((bp->b_flags & B_READ) == 0) {
		bp->b_error = EOPNOTSUPP;
		goto bad;
	}

	off = (u_int64_t)bp->b_blkno * DEV_BSIZE;
	if (off < tmpfsrd_read || off > tmpfsrd_size) {
		bp->b_error = ENXIO;
		goto bad;
	}

	len = bp->b_bcount;
	if (len > tmpfsrd_size - off)
		len = tmpfsrd_size - off;

	memcpy(bp->b_data, tmpfsrd_disk + off, len);

	bp->b_resid = bp->b_bcount - len;
	tmpfsrd_read += len;

	/*
	 * On systems with limited memory, we could pmap_kremove() here when
	 * crossing pages, at the cost of a higher number of TLB flushes.
	 */

	goto done;

bad:
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;

done:
	s = splbio();
	biodone(bp);
	splx(s);
	if (sc != NULL)
		device_unref(&sc->sc_dev);
}

int
tmpfsrdioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	int unit = DISKUNIT(dev), error = 0;
	struct tmpfsrd_softc *sc = (void *)disk_lookup(&tmpfsrd_cd, unit);

	if (sc == NULL)
		return (ENXIO);

	switch (cmd) {
	case DIOCRLDINFO:
		/* Fake it; pretend the disklabel was reloaded. */
		break;
	case DIOCGPDINFO:
		tmpfsrdgetdisklabel(dev, sc, (struct disklabel *)data, 1);
		break;
	case DIOCGDINFO:
		*(struct disklabel *)data = *(sc->sc_dk.dk_label);
		break;
	default:
		error = ENOTTY;
	}

	device_unref(&sc->sc_dev);

	return (error);
}

int
tmpfsrdgetdisklabel(dev_t dev, struct tmpfsrd_softc *sc, struct disklabel *lp,
    int spoofonly)
{
	bzero(lp, sizeof(*lp));

	/* Start with some basics. All the hardcoded values are fake. */
	lp->d_type = DTYPE_SCSI;
	lp->d_secsize = DEV_BSIZE;
	lp->d_version = 1;
	lp->d_ntracks = 1;
	lp->d_ncylinders = 1;
	lp->d_bbsize = 8192;	/* Arbitrary. */
	lp->d_sbsize = 64*1024;	/* Arbitrary. */

	strncpy(lp->d_typename, "tmpfs RAM disk", sizeof(lp->d_typename));
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));

	/* Derive the file system geometry from 'tmpfsrd_size'. */
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_nsectors = tmpfsrd_size >> DEV_BSHIFT;
	lp->d_secpercyl = lp->d_nsectors;
	DL_SETDSIZE(lp, lp->d_nsectors);

	/* Fill in partition 'a'. */
	DL_SETPSIZE(&lp->d_partitions[0], DL_GETDSIZE(lp));
	lp->d_partitions[0].p_fstype = FS_TMPFS;

	/* Fill in partition 'c'. */
	DL_SETPSIZE(&lp->d_partitions[RAW_PART], DL_GETDSIZE(lp));

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;

	lp->d_checksum = dkcksum(lp);

	return (0);
}

int
tmpfsrdread(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(tmpfsrdstrategy, dev, B_READ, minphys, uio));
}

int
tmpfsrdwrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(tmpfsrdstrategy, dev, B_WRITE, minphys, uio));
}

int
tmpfsrddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	return (ENXIO);
}

daddr_t
tmpfsrdsize(dev_t dev)
{
	return (-1);
}
