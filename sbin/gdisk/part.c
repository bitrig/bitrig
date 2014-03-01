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
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <err.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"
#include "misc.h"
#include "gpt.h"

static const struct part_type {
	int	type;
	char	sguid[38];
	char	sname[38];
	int	display;
} part_types[] = {
	{ 0x0000, "00000000-0000-0000-0000-000000000000", "Unused", 0 },

	/* DOS/Windows partition types, most of which are hidden from the "L" listing */
	/* (they're available mainly for MBR-to-GPT conversions). */
	{ 0x0100, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-12 */
	{ 0x0400, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-16 < 32M */
	{ 0x0600, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-16 */
	{ 0x0700, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 1 }, /* NTFS (or HPFS) */
	{ 0x0b00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-32 */
	{ 0x0c00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-32 LBA */
	{ 0x0c01, "e3c9e316-0b5c-4db8-817d-f92df00215ae", "Microsoft reserved" },
	{ 0x0e00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* FAT-16 LBA */
	{ 0x1100, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-12 */
	{ 0x1400, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-16 < 32M */
	{ 0x1600, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-16 */
	{ 0x1700, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden NTFS (or HPFS) */
	{ 0x1b00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-32 */
	{ 0x1c00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-32 LBA */
	{ 0x1e00, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "Microsoft basic data", 0 }, /* Hidden FAT-16 LBA */
	{ 0x2700, "de94bba4-06d1-4d40-a16a-bfd50179d6ac", "Windows RE" },
	{ 0x4200, "af9b60a0-1431-4f62-bc68-3311714a69ad", "Windows LDM data" }, /* Logical disk manager */
	{ 0x4201, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3", "Windows LDM metadata" }, /* Logical disk manager */

	/* IBM filesystem */
	{ 0x7501, "37affc90-ef7d-4e96-91c3-2d7ae055b174", "IBM GPFS" }, /* General Parallel File System (GPFS) */

	/* ChromeOS-specific partition types */
	{ 0x7f00, "fe3a2a5d-4f32-41a7-b725-accc3285a309", "ChromeOS kernel" },
	{ 0x7f01, "3cb8e202-3b7e-47dd-8a3c-7ff2a13cfcec", "ChromeOS root" },
	{ 0x7f02, "2e0a753d-9e48-43b0-8337-b15192cb1b5e", "ChromeOS reserved" },

	/* Linux-specific partition types */
	{ 0x8200, "0657fd6d-a4ab-43c4-84e5-0933c84b4f4f", "Linux swap" }, /* Linux swap (or Solaris on MBR) */
	{ 0x8300, "0fc63daf-8483-4772-8e79-3d69d8477de4", "Linux filesystem" }, /* Linux native */
	{ 0x8301, "8da63339-0007-60c0-c436-083ac8230908", "Linux reserved" },
	{ 0x8e00, "e6d6d379-f507-44c2-a23c-238f2a3df928", "Linux LVM" },

	/* FreeBSD partition types */
	{ 0xa500, "516e7cb4-6ecf-11d6-8ff8-00022d09712b", "FreeBSD disklabel" },
	{ 0xa501, "83bd6b9d-7f41-11dc-be0b-001560b84f0f", "FreeBSD boot" },
	{ 0xa502, "516e7cb5-6ecf-11d6-8ff8-00022d09712b", "FreeBSD swap" },
	{ 0xa503, "516e7cb6-6ecf-11d6-8ff8-00022d09712b", "FreeBSD UFS" },
	{ 0xa504, "516e7cba-6ecf-11d6-8ff8-00022d09712b", "FreeBSD ZFS" },
	{ 0xa505, "516e7cb8-6ecf-11d6-8ff8-00022d09712b", "FreeBSD Vinum/RAID" },

	/* Midnight BSD partition types */
	{ 0xa580, "85d5e45a-237c-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD data" },
	{ 0xa581, "85d5e45e-237c-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD boot" },
	{ 0xa582, "85d5e45b-237c-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD swap" },
	{ 0xa583, "0394ef8b-237e-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD UFS" },
	{ 0xa584, "85d5e45d-237c-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD ZFS" },
	{ 0xa585, "85d5e45c-237c-11e1-b4b3-e89a8f7fc3a7", "Midnight BSD Vinum" },

	/* OpenBSD partition types */
	{ 0xa600, "824cc7a0-36a8-11e3-890a-952519ad3f61", "OpenBSD disklabel" },

	/* MacOS partition types */
	{ 0xa800, "55465300-0000-11aa-aa11-00306543ecac", "Apple UFS" },

	/* NetBSD partition types. XXX: FreeBSD disklabel? */
	{ 0xa900, "516e7cb4-6ecf-11d6-8ff8-00022d09712b", "FreeBSD disklabel", 0 },
	{ 0xa901, "49f48d32-b10e-11dc-b99b-0019d1879648", "NetBSD swap" },
	{ 0xa902, "49f48d5a-b10e-11dc-b99b-0019d1879648", "NetBSD FFS" },
	{ 0xa903, "49f48d82-b10e-11dc-b99b-0019d1879648", "NetBSD LFS" },
	{ 0xa904, "2db519c4-b10f-11dc-b99b-0019d1879648", "NetBSD concatenated" },
	{ 0xa905, "2db519ec-b10f-11dc-b99b-0019d1879648", "NetBSD encrypted" },
	{ 0xa906, "49f48daa-b10e-11dc-b99b-0019d1879648", "NetBSD RAID" },

	/* MacOS partition types */
	{ 0xab00, "426f6f74-0000-11aa-aa11-00306543ecac", "Apple boot" },
	{ 0xaf00, "48465300-0000-11aa-aa11-00306543ecac", "Apple HFS/HFS+" },
	{ 0xaf01, "52414944-0000-11aa-aa11-00306543ecac", "Apple RAID" },
	{ 0xaf02, "52414944-5f4f-11aa-aa11-00306543ecac", "Apple RAID offline" },
	{ 0xaf03, "4c616265-6c00-11aa-aa11-00306543ecac", "Apple label" },
	{ 0xaf04, "5265636f-7665-11aa-aa11-00306543ecac", "AppleTV recovery" },
	{ 0xaf05, "53746f72-6167-11aa-aa11-00306543ecac", "Apple Core Storage" },

	/* Solaris partition types */
	{ 0xbe00, "6a82cb45-1dd2-11b2-99a6-080020736631", "Solaris boot" },
	{ 0xbf00, "6a85cf4d-1dd2-11b2-99a6-080020736631", "Solaris root" },
	{ 0xbf01, "6a898cc3-1dd2-11b2-99a6-080020736631", "Solaris /usr & Mac ZFS" }, /* Solaris/MacOS */
	{ 0xbf02, "6a87c46f-1dd2-11b2-99a6-080020736631", "Solaris swap" },
	{ 0xbf03, "6a8b642b-1dd2-11b2-99a6-080020736631", "Solaris backup" },
	{ 0xbf04, "6a8ef2e9-1dd2-11b2-99a6-080020736631", "Solaris /var" },
	{ 0xbf05, "6a90ba39-1dd2-11b2-99a6-080020736631", "Solaris /home" },
	{ 0xbf06, "6a9283a5-1dd2-11b2-99a6-080020736631", "Solaris alternate sector" },
	{ 0xbf07, "6a945a3b-1dd2-11b2-99a6-080020736631", "Solaris Reserved 1" },
	{ 0xbf08, "6a9630d1-1dd2-11b2-99a6-080020736631", "Solaris Reserved 2" },
	{ 0xbf09, "6a980767-1dd2-11b2-99a6-080020736631", "Solaris Reserved 3" },
	{ 0xbf0a, "6a96237f-1dd2-11b2-99a6-080020736631", "Solaris Reserved 4" },
	{ 0xbf0b, "6a8d2ac7-1dd2-11b2-99a6-080020736631", "Solaris Reserved 5" },

	/* HP-UX partition types */
	{ 0xc001, "75894c1e-3aeb-11d3-b7c1-7b03a0000000", "HP-UX data" },
	{ 0xc002, "e2a1e728-32e3-11d6-a682-7b03a0000000", "HP-UX service" },

	/* Haiku partition types; uses BeOS MBR code as hex code base */
	{ 0xeb00, "42465331-3ba3-10f1-802a-4861696b7521", "Haiku BFS" },

	/* Sony partition types */
	{ 0xed00, "f4019732-066e-4e12-8273-346c5641494f", "Sony system partition" },

	/* EFI system and related partitions */
	{ 0xef00, "c12a7328-f81f-11d2-ba4b-00a0c93ec93b", "EFI System" }, /* Parted identifies these as having the "boot flag" set */
	{ 0xef01, "024dee41-33e7-11d3-9d69-0008c781f39f", "MBR partition scheme" }, /* Used to nest MBR in GPT */
	{ 0xef02, "21686148-6449-6e6f-744e-656564454649", "BIOS boot partition" }, /* Used by GRUB */
	{ 0xef03, "d3bfe2de-3daf-11df-ba40-e3a556d89593", "Intel Fast Flash" }, /* Used by Intel Rapid Start technology */

	/* VMWare ESX partition types */
	{ 0xfb00, "aa31e02a-400f-11db-9590-000c2911d1b8", "VMWare VMFS" },
	{ 0xfb01, "9198effc-31c0-11db-8f78-000c2911d1b8", "VMWare reserved" },
	{ 0xfc00, "9d275380-40ad-11db-bf97-000c2911d1b8", "VMWare kcore crash protection" },

	/* Linux partition types */
	{ 0xfd00, "a19d880f-05fc-4d3b-a006-743f0f84911e", "Linux RAID" },
};

void
PRT_printall(void)
{
	int i, idrows;

	idrows = ((sizeof(part_types)/sizeof(struct part_type))+2)/3;

	printf("Choose from the following Partition id values:\n");
	for (i = 0; i < idrows; i++) {
		printf("%04X %24s   %04X %24s",
		    part_types[i].type, part_types[i].sname,
		    part_types[i+idrows].type, part_types[i+idrows].sname);
		if ((i+idrows*2) < (sizeof(part_types)/sizeof(struct part_type))) {
			printf("   %04X %24s\n",
			    part_types[i+idrows*2].type,
			    part_types[i+idrows*2].sname);
		} else
			printf( "\n" );
	}
}

const char *
PRT_ascii_id(int id)
{
	static char unknown[] = "<Unknown ID>";
	int i;

	for (i = 0; i < sizeof(part_types)/sizeof(struct part_type); i++) {
		if (part_types[i].type == id)
			return (part_types[i].sname);
	}

	return (unknown);
}

const char *
PRT_ascii_name(uint16_t *utf)
{
	static char name[36];
	int i;

	for (i = 0; i < 36; i++)
		name[i] = utf[i] & 0x7F;

	return name;
}

const char *
PRT_ascii_type(uuid_t *guid)
{
	static char unknown[] = "<Unknown ID>";
	int i;
	char *string;

	uuid_to_string(guid, &string, NULL);

	for (i = 0; i < sizeof(part_types)/sizeof(struct part_type); i++) {
		if (!strncmp(part_types[i].sguid, string, 36)) {
			free(string);
			return (part_types[i].sname);
		}
	}

	free(string);
	return (unknown);
}

void
PRT_set_guid(uuid_t *guid, const char *buf)
{
	if (strlen(buf) != 36)
		return;

	sscanf(buf, "%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
	    &guid->time_low, &guid->time_mid, &guid->time_hi_and_version,
	    &guid->clock_seq_hi_and_reserved, &guid->clock_seq_low,
	    &guid->node[0], &guid->node[1], &guid->node[2],
	    &guid->node[3], &guid->node[4], &guid->node[5]);
}

int
PRT_pid_for_type(uuid_t *guid)
{
	int i;
	char *string;

	uuid_to_string(guid, &string, NULL);

	for (i = 0; i < sizeof(part_types)/sizeof(struct part_type); i++) {
		if (!strncmp(part_types[i].sguid, string, 36)) {
			free(string);
			return (part_types[i].type);
		}
	}

	free(string);
	return 0;
}

void
PRT_set_type_by_pid(gpt_partition_t *partn, int pid)
{
	int i;

	for (i = 0; i < sizeof(part_types)/sizeof(struct part_type); i++) {
		if (part_types[i].type == pid)
			return uuid_from_string(part_types[i].sguid, &partn->type, NULL);
	}
}

void
PRT_print(int num, gpt_partition_t *partn, char *units)
{
	double size;
	int i;
	i = unit_lookup(units);

	if (partn == NULL) {
		printf("                                                      LBA Info:\n");
		printf("   #:                 type name                 [       start:        size ]\n");
		printf("-------------------------------------------------------------------------------\n");
	} else {
		size = partn->end_lba - partn->start_lba + 1;
		size = ((double)size * unit_types[SECTORS].conversion) /
		    unit_types[i].conversion;
		printf("%c%3d: %20s %-20s [%12lld:%12.0f%s]\n",
		    (partn->attribute == 0x02)?'*':' ',
		    num,
		    PRT_ascii_type(&partn->type),
		    PRT_ascii_name(partn->name),
		    partn->start_lba, size,
		    unit_types[i].abbr
		    );
	}
}

int
PRT_overlap(gpt_partition_t *p1, gpt_partition_t *p2)
{
	return (p1->end_lba >= p2->start_lba && p2->end_lba >= p1->start_lba);
}
