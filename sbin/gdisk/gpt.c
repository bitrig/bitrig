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
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <err.h>
#include <errno.h>
#include <util.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <zlib.h>
#include "disk.h"
#include "misc.h"
#include "gpt.h"
#include "part.h"

int
GPT_init(disk_t *disk, gpt_t *gpt)
{
	uint64_t i;

	/* free existing metadata, redo them */
	if (gpt->header)
		free(gpt->header);

	if (gpt->part)
		free(gpt->part);

	gpt->header = malloc(sizeof(gpt_header_t));
	if (gpt->header == NULL)
		return (-ENOMEM);
	memset(gpt->header, 0, sizeof(gpt_header_t));

	gpt->part = malloc(GPT_PARTITIONS * sizeof(gpt_partition_t));
	if (gpt->part == NULL)
		return (-ENOMEM);
	memset(gpt->part, 0, GPT_PARTITIONS * sizeof(gpt_partition_t));

	/* Setup just the basics. */
	gpt->header->partitions_num = GPT_PARTITIONS;
	gpt->header->current_lba = 1;
	gpt->header->backup_lba = disk->real->size - 1;
	gpt->header->start_lba = (gpt->header->partitions_num + 3) / 4 + 2;
	gpt->header->end_lba = disk->real->size -
	    ((gpt->header->partitions_num + 3) / 4 + 2);
	uuidgen(&gpt->header->guid, 1);

	/* Start OpenBSD GPT partition on a power of 2 block number. */
	i = 1;
	while (i < DL_SECTOBLK(&dl, gpt->header->start_lba))
		i *= 2;

	gpt->part[0].start_lba = i;
	gpt->part[0].end_lba = gpt->header->end_lba;

	PRT_set_type_by_pid(&gpt->part[0], GPTPTYP_OPENBSD);

	return 0;
}

void
GPT_print(gpt_t *gpt, char *units)
{
	int i;
	char *uuid;

	/* Header */
	uuid_to_string(&gpt->header->guid, &uuid, NULL);
	printf("Disk UUID: %s\n", uuid);
	PRT_print(0, NULL, units);

	/* Entries */
	for (i = 0; i < gpt->header->partitions_num; i++)
		if (gpt->part[i].type.time_low || gpt->part[i].type.time_mid ||
		    gpt->part[i].type.time_hi_and_version ||
		    gpt->part[i].type.clock_seq_hi_and_reserved ||
		    gpt->part[i].type.clock_seq_low ||
		    gpt->part[i].type.node[0] ||
		    gpt->part[i].type.node[1] ||
		    gpt->part[i].type.node[2] ||
		    gpt->part[i].type.node[3] ||
		    gpt->part[i].type.node[4] ||
		    gpt->part[i].type.node[5])
			PRT_print(i, &gpt->part[i], units);
}

int
GPT_read(int fd, off_t where, char *buf)
{
	const int secsize = unit_types[SECTORS].conversion;
	ssize_t len;
	off_t off;
	char *secbuf;

	where *= secsize;
	off = lseek(fd, where, SEEK_SET);
	if (off != where)
		return (-1);

	secbuf = malloc(secsize);
	if (secbuf == NULL)
		return (-1);
	bzero(secbuf, secsize);

	len = read(fd, secbuf, secsize);
	bcopy(secbuf, buf, DEV_BSIZE);
	free(secbuf);

	if (len == -1)
		return (-1);
	if (len < DEV_BSIZE) {
		/* short read */
		errno = EIO;
		return (-1);
	}

	return (0);
}

int
GPT_write(int fd, off_t where, char *buf)
{
	const int secsize = unit_types[SECTORS].conversion;
	ssize_t len;
	off_t off;
	char *secbuf;

	/* Read the sector we want to store the GPT in. */
	where *= secsize;
	off = lseek(fd, where, SEEK_SET);
	if (off != where)
		return (-1);

	secbuf = malloc(secsize);
	if (secbuf == NULL)
		return (-1);
	bzero(secbuf, secsize);

	len = read(fd, secbuf, secsize);
	if (len == -1 || len != secsize)
		goto done;

	/*
	 * Place the new GPT in the first DEV_BSIZE bytes of the sector and
	 * write the sector back to "disk".
	 */
	bcopy(buf, secbuf, DEV_BSIZE);
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

int
GPT_load(int fd, disk_t *disk, gpt_t *gpt, int alt)
{
	int i;
	int offset = 1;
	char buf[DEV_BSIZE];
	uint32_t crc;

	gpt->header = malloc(sizeof(gpt_header_t));
	if (gpt->header == NULL)
		goto error;

	if (offset >= disk->real->size) {
		printf("Disk too small!\n");
		goto header;
	}

	if (GPT_read(fd, offset, buf))
		goto header;

	memcpy(gpt->header, buf, sizeof(gpt_header_t));

	if (alt) {
		offset = gpt->header->backup_lba;

		if (offset >= disk->real->size) {
			printf("Disk too small!\n");
			goto header;
		}

		if (GPT_read(fd, offset, buf))
			goto header;

		memcpy(gpt->header, buf, sizeof(gpt_header_t));
	}

	if (gpt->header->signature != GPT_SIGNATURE) {
		printf("Signature does not match!\n");
		goto header;
	}

	if (gpt->header->header_size > sizeof(gpt_header_t)) {
		printf("Header size too big!\n");
		goto header;
	}

	/* save crc and set the field to zero for calculation */
	crc = gpt->header->header_checksum;
	gpt->header->header_checksum = 0;

	gpt->header->header_checksum = crc32(0,
	    (unsigned char *)gpt->header, gpt->header->header_size);

	if (gpt->header->header_checksum != crc) {
		printf("Header checksum does not match!\n");
		goto header;
	}

	if (gpt->header->partitions_size != sizeof(gpt_partition_t)) {
		printf("Partition Size does not match!\n");
		goto header;
	}

	gpt->part = malloc(gpt->header->partitions_num * sizeof(gpt_partition_t));
	if (gpt->part == NULL)
		goto header;

	for (i = 0; i < gpt->header->partitions_num; i++) {
		if (i % 4 == 0)
			if (GPT_read(fd, gpt->header->partitions_lba + (i / 4), buf))
				goto partitions;
		memcpy(&gpt->part[i],
		    buf + (i % 4) * sizeof(gpt_partition_t),
		    sizeof(gpt_partition_t));
	}

	/* save crc and set the field to zero for calculation */
	crc = gpt->header->partitions_checksum;
	gpt->header->partitions_checksum = 0;

	gpt->header->partitions_checksum = crc32(0,
	    (unsigned char *)gpt->part, gpt->header->partitions_num *
	    gpt->header->partitions_size);

	if (gpt->header->partitions_checksum != crc) {
		printf("Partition checksum does not match!\n");
		goto partitions;
	}

	return 0;

partitions:
	free(gpt->part);
header:
	free(gpt->header);
error:
	return -1;
}

void
GPT_free(gpt_t *gpt)
{
	free(gpt->header);
	free(gpt->part);
}

int
GPT_store(int fd, disk_t *disk, gpt_t *gpt)
{
	int i, need;
	char gpt_buf[DEV_BSIZE];

	/* First, sanity checks. */
	if (gpt->header->current_lba != 1 &&
	    gpt->header->backup_lba != 1) {
		printf("GPT headers don't use LBA 1!\n");
		return -1;
	}

	/* Amount of LBA's needed. */
	need = 1 + (gpt->header->partitions_num + 3) / 4;

	gpt->header->start_lba = need + 1;
	gpt->header->end_lba = disk->real->size - (need + 1);

	if (disk->real->size < need+need+1) {
		printf("Disk too small\n");
		return -1;
	}

	for (i = 0; i < gpt->header->partitions_num; i++) {
		if (uuid_is_nil(&gpt->part[i].type, NULL))
			continue;
		if (gpt->part[i].start_lba < gpt->header->start_lba ||
		    gpt->part[i].start_lba > gpt->part[i].end_lba ||
		    gpt->part[i].end_lba > gpt->header->end_lba) {
			printf("Partition %i is out of bounds!\n", i);
			return -1;
		}
	}

	/*
	 * Basic setup, but leave guid and # of partitionsalone.
	 * Also, start- and end-LBA are already set.
	 */
	gpt->header->signature = GPT_SIGNATURE;
	gpt->header->revision = GPT_REVISION;
	gpt->header->header_size = sizeof(gpt_header_t);
	gpt->header->header_checksum = 0;
	gpt->header->reserved = 0;
	gpt->header->current_lba = 1;
	gpt->header->backup_lba = disk->real->size - 1;
	gpt->header->partitions_lba = 2;
	gpt->header->partitions_size = sizeof(gpt_partition_t);

	gpt->header->partitions_checksum = crc32(0,
	    (unsigned char *)gpt->part, gpt->header->partitions_num *
	    gpt->header->partitions_size);

	gpt->header->header_checksum = crc32(0,
	    (unsigned char *)gpt->header, gpt->header->header_size);

	/* All set, prepare the writing. */
	memset(gpt_buf, 0, DEV_BSIZE);
	memcpy(gpt_buf, gpt->header, sizeof(gpt_header_t));
	if (GPT_write(fd, gpt->header->current_lba, gpt_buf) == -1)
		goto error;

	memset(gpt_buf, 0, DEV_BSIZE);
	for (i = 0; i < gpt->header->partitions_num; i++) {
		memcpy(gpt_buf + (i % 4) * sizeof(gpt_partition_t),
		    &gpt->part[i], sizeof(gpt_partition_t));

		if (i % 4 == 3) {
			if (GPT_write(fd, gpt->header->partitions_lba + (i / 4),
			    gpt_buf) == -1)
				goto error;
			memset(gpt_buf, 0, DEV_BSIZE);
		}
	}

	/* If the last one wasn't written, due to uneven entries, write it. */
	if (i % 4 != 0)
		if (GPT_write(fd, gpt->header->partitions_lba + (i / 4),
			    gpt_buf) == -1)
			goto error;

	gpt->header->current_lba = disk->real->size - 1;
	gpt->header->backup_lba = 1;
	gpt->header->partitions_lba = gpt->header->end_lba + 1;
	gpt->header->header_checksum = 0;

	gpt->header->header_checksum = crc32(0,
	    (unsigned char *)gpt->header, gpt->header->header_size);

	/* All set, prepare the writing. */
	memset(gpt_buf, 0, DEV_BSIZE);
	memcpy(gpt_buf, gpt->header, sizeof(gpt_header_t));
	if (GPT_write(fd, gpt->header->current_lba, gpt_buf) == -1)
		goto error;

	memset(gpt_buf, 0, DEV_BSIZE);
	for (i = 0; i < gpt->header->partitions_num; i++) {
		memcpy(gpt_buf + (i % 4) * sizeof(gpt_partition_t),
		    &gpt->part[i], sizeof(gpt_partition_t));

		if (i % 4 == 3) {
			if (GPT_write(fd, gpt->header->partitions_lba + (i / 4),
			    gpt_buf) == -1)
				goto error;
			memset(gpt_buf, 0, DEV_BSIZE);
		}
	}

	/* If the last one wasn't written, due to uneven entries, write it. */
	if (i % 4 != 0)
		if (GPT_write(fd, gpt->header->partitions_lba + (i / 4),
			    gpt_buf) == -1)
			goto error;

	return 0;
error:
	printf("Couldn't write GPT\n");
	return -1;
}

int
GPT_verify(gpt_t *gpt)
{
	uint32_t i, j, n;
	gpt_partition_t *p1, *p2;

	for (i = 0, n = 0; i < gpt->header->partitions_num; i++) {
		p1 = &gpt->part[i];

		if (uuid_is_nil(&p1->type, NULL))
			continue;

		if (PRT_pid_for_type(&p1->type) == GPTPTYP_OPENBSD)
			n++;

		for (j = i + 1; j < gpt->header->partitions_num; j++) {
			p2 = &gpt->part[j];
			if (!uuid_is_nil(&p2->type, NULL) &&
			    PRT_overlap(p1, p2)) {
				warnx("Partitions %u and %u are overlapping!", i, j);
				if (!ask_yn("Write GPT anyway?"))
					return (-1);
			}

		}

	}
	if (n >= 2) {
		warnx("GPT contains more than one OpenBSD partition!");
		if (!ask_yn("Write GPT anyway?"))
			return (-1);

	}

	return (0);
}
