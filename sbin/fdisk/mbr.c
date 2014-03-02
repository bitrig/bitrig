/*	$OpenBSD: mbr.c,v 1.38 2014/03/17 16:40:00 krw Exp $	*/

/*
 * Copyright (c) 1997 Tobias Weingartner
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <err.h>
#include <errno.h>
#include <util.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include "disk.h"
#include "part.h"
#include "misc.h"
#include "mbr.h"
#include "part.h"

void
MBR_init_GPT(struct disk *disk, struct mbr *mbr)
{
	/* initialize a protective MBR for GPT */
	bzero(&mbr->part, sizeof(mbr->part));

	/* Use whole disk, starting after MBR. */
	mbr->part[0].id = DOSPTYP_EFI;
	mbr->part[0].bs = 1;
	mbr->part[0].ns = disk->size - 1;

	/* Fix up start/length fields */
	PRT_fix_CHS(disk, &mbr->part[0]);
}

void
MBR_init(struct disk *disk, struct mbr *mbr)
{
	extern int g_flag;

	if (g_flag) {
		MBR_init_GPT(disk, mbr);
		return;
	}

	/* Fix up given mbr for this disk */
	mbr->part[0].flag = 0;
	mbr->part[1].flag = 0;
	mbr->part[2].flag = 0;
	mbr->part[3].flag = DOSACTIVE;
	mbr->part[3].id = DOSPTYP_OPENBSD;
	mbr->signature = DOSMBR_SIGNATURE;

#if defined(__powerpc__) || defined(__mips__)
	/* Now fix up for the MS-DOS boot partition on PowerPC. */
	mbr->part[0].flag = DOSACTIVE;	/* Boot from dos part */
	mbr->part[3].flag = 0;
#endif

	MBR_fillremaining(mbr, disk, 3);
}

void
MBR_parse(struct disk *disk, struct dos_mbr *dos_mbr, off_t offset,
    off_t reloff, struct mbr *mbr)
{
	int i;

	memcpy(mbr->code, dos_mbr->dmbr_boot, sizeof(mbr->code));
	mbr->offset = offset;
	mbr->reloffset = reloff;
	mbr->signature = getshort(&dos_mbr->dmbr_sign);

	for (i = 0; i < NDOSPART; i++)
		PRT_parse(disk, &dos_mbr->dmbr_parts[i], offset, reloff,
		    &mbr->part[i]);
}

void
MBR_make(struct mbr *mbr, struct dos_mbr *dos_mbr)
{
	int i;

	memcpy(dos_mbr->dmbr_boot, mbr->code, sizeof(dos_mbr->dmbr_boot));
	putshort(&dos_mbr->dmbr_sign, DOSMBR_SIGNATURE);

	for (i = 0; i < NDOSPART; i++)
		PRT_make(&mbr->part[i], mbr->offset, mbr->reloffset,
		    &dos_mbr->dmbr_parts[i]);
}

void
MBR_print(struct mbr *mbr, char *units)
{
	int i;

	/* Header */
	printf("Signature: 0x%X\n", (int)mbr->signature);
	PRT_print(0, NULL, units);

	/* Entries */
	for (i = 0; i < NDOSPART; i++)
		PRT_print(i, &mbr->part[i], units);
}

int
MBR_read(int fd, off_t where, struct dos_mbr *dos_mbr)
{
	const int secsize = unit_types[SECTORS].conversion;
	ssize_t len;
	off_t off;
	char *secbuf;

	where *= secsize;
	off = lseek(fd, where, SEEK_SET);
	if (off != where)
		return (-1);

	secbuf = calloc(1, secsize);
	if (secbuf == NULL)
		return (-1);

	len = read(fd, secbuf, secsize);
	memcpy(dos_mbr, secbuf, sizeof(*dos_mbr));
	free(secbuf);

	if (len == -1)
		return (-1);
	if (len < sizeof(*dos_mbr)) {
		/* short read */
		errno = EIO;
		return (-1);
	}

	return (0);
}

int
MBR_write(int fd, off_t where, struct dos_mbr *dos_mbr)
{
	const int secsize = unit_types[SECTORS].conversion;
	ssize_t len;
	off_t off;
	char *secbuf;

	/* Read the sector we want to store the MBR in. */
	where *= secsize;
	off = lseek(fd, where, SEEK_SET);
	if (off != where)
		return (-1);

	secbuf = calloc(1, secsize);
	if (secbuf == NULL)
		return (-1);

	len = read(fd, secbuf, secsize);
	if (len == -1 || len != secsize)
		goto done;

	/*
	 * Place the new MBR at the start of the sector and
	 * write the sector back to "disk".
	 */
	memcpy(secbuf, dos_mbr, sizeof(*dos_mbr));
	off = lseek(fd, where, SEEK_SET);
	if (off == where)
		len = write(fd, secbuf, secsize);
	else
		len = -1;

done:
	free(secbuf);
	if (len == -1)
		return (-1);
	if (len != secsize) {
		/* short read or write */
		errno = EIO;
		return (-1);
	}

	ioctl(fd, DIOCRLDINFO, 0);
	return (0);
}

/*
 * Parse the MBR partition table into 'mbr', leaving the rest of 'mbr'
 * untouched.
 */
void
MBR_pcopy(struct disk *disk, struct mbr *mbr)
{
	int i, fd, error;
	struct dos_mbr dos_mbr;

	fd = DISK_open(disk->name, O_RDONLY);
	error = MBR_read(fd, 0, &dos_mbr);
	close(fd);

	if (error == -1)
		return;

	for (i = 0; i < NDOSPART; i++)
		PRT_parse(disk, &dos_mbr.dmbr_parts[i], 0, 0, &mbr->part[i]);
}

int
MBR_verify(struct mbr *mbr)
{
	int i, j, n;
	struct prt *p1, *p2;

	for (i = 0, n = 0; i < NDOSPART; i++) {
		p1 = &mbr->part[i];
		if (p1->id == DOSPTYP_UNUSED)
			continue;

		if (p1->id == DOSPTYP_OPENBSD)
			n++;

		for (j = i + 1; j < NDOSPART; j++) {
			p2 = &mbr->part[j];
			if (p2->id != DOSPTYP_UNUSED && PRT_overlap(p1, p2)) {
				warnx("Partitions %d and %d are overlapping!", i ,j);
				if (!ask_yn("Write MBR anyway?"))
					return (-1);
			}
		}
	}
	if (n >= 2) {
		warnx("MBR contains more than one OpenBSD partition!");
		if (!ask_yn("Write MBR anyway?"))
			return (-1);
	}

	return (0);
}

void
MBR_fillremaining(struct mbr *mbr, struct disk *disk, int pn)
{
	struct prt *part, *p;
	u_int64_t adj;
	daddr_t i;

	part = &mbr->part[pn];

	/* Use whole disk. Reserve first track, or first cyl, if possible. */
	if (disk->heads > 1)
		part->shead = 1;
	else
		part->shead = 0;
	if (disk->heads < 2 && disk->cylinders > 1)
		part->scyl = 1;
	else
		part->scyl = 0;
	part->ssect = 1;

	/* Go right to the end */
	part->ecyl = disk->cylinders - 1;
	part->ehead = disk->heads - 1;
	part->esect = disk->sectors;

	/* Fix up start/length fields */
	PRT_fix_BN(disk, part, pn);

#if defined(__powerpc__) || defined(__mips__)
	if ((part->shead != 1) || (part->ssect != 1)) {
		/* align the partition on a cylinder boundary */
		part->shead = 0;
		part->ssect = 1;
		part->scyl += 1;
	}
	/* Fix up start/length fields */
	PRT_fix_BN(disk, part, pn);
#endif

	/* Start OpenBSD MBR partition on a power of 2 block number. */
	i = 1;
	while (i < DL_SECTOBLK(&dl, part->bs))
		i *= 2;
	adj = DL_BLKTOSEC(&dl, i) - part->bs;
	part->bs += adj;
	part->ns -= adj;
	PRT_fix_CHS(disk, part);

	/* Shrink to remaining free space */
	for (i = 0; i < NDOSPART; i++) {
		p = &mbr->part[i];
		if (i != pn && PRT_overlap(part, p)) {
			if (p->bs > part->bs) {
				part->ns = p->bs - part->bs;
			} else {
				part->ns += part->bs;
				part->bs = p->bs + p->ns;
				part->ns -= part->bs;
			}
		}
	}
	PRT_fix_CHS(disk, part);
}
