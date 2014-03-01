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

#ifndef _GPT_H
#define _GPT_H

#include "part.h"

/* Various constants */
#define GPT_SIGNATURE	0x5452415020494645
#define GPT_REVISION	0x10000
#define GPT_PARTITIONS	128
#define GPTPTYP_OPENBSD	0xa600

#define GPT_DOSACTIVE		0x2
#define GPT_PRIORITY_SHIFT	48
#define GPT_PRIORITY_MASK	0xfULL
#define GPT_PRIORITY_MAX	(GPT_PRIORITY_MASK)
#define GPT_TRIES_SHIFT		52
#define GPT_TRIES_MASK		0xfULL
#define GPT_TRIES_MAX		(GPT_TRIES_MASK)
#define GPT_SUCCESS_SHIFT	56
#define GPT_SUCCESS_MASK	0x1ULL
#define GPT_SUCCESS_MAX		(GPT_SUCCESS_MASK)

/* GPT header */
typedef struct __attribute__((packed)) _gpt_header_t {
	uint64_t signature;			/* "EFI PART" */
	uint32_t revision;			/* GPT Version 1.0: 0x00000100 */
	uint32_t header_size;			/* Little-Endian */
	uint32_t header_checksum;		/* CRC32: with this field as 0 */
	uint32_t reserved;			/* always zero */
	uint64_t current_lba;			/* location of this header */
	uint64_t backup_lba;			/* location of backup header */
	uint64_t start_lba;			/* first usable LBA (after this header) */
	uint64_t end_lba;			/* last usable LBA (before backup header) */
	uuid_t   guid;				/* disk GUID / UUID */
	uint64_t partitions_lba;		/* location of partition entries */
	uint32_t partitions_num;		/* # of reserved entry space, usually 128 */
	uint32_t partitions_size;		/* size per entry */
	uint32_t partitions_checksum;		/* checksum of all entries */
} gpt_header_t;

/* GPT data struct */
typedef struct _gpt_t {
	gpt_header_t *header;
	gpt_partition_t *part;
} gpt_t;

/* Prototypes */
void GPT_print(gpt_t *, char *);
int GPT_init(disk_t *, gpt_t *);
int GPT_load(int, disk_t *, gpt_t *, int);
int GPT_store(int, disk_t *, gpt_t *);
int GPT_read(int, off_t, char *);
int GPT_write(int, off_t, char *);
int GPT_verify(gpt_t *);
void GPT_free(gpt_t *);

/* Sanity check */
#include <sys/param.h>
#if (DEV_BSIZE != 512)
#error "DEV_BSIZE != 512, somebody better fix me!"
#endif

#endif /* _GPT_H */

