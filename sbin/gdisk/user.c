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
#include <errno.h>
#include <util.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "user.h"
#include "disk.h"
#include "misc.h"
#include "gpt.h"
#include "cmd.h"


/* Our command table */
static cmd_table_t cmd_table[] = {
	{"help",   Xhelp,	"Command help list"},
	{"manual", Xmanual,	"Show entire OpenBSD man page for gdisk"},
	{"reinit", Xreinit,	"Re-initialize loaded GPT (to defaults)"},
	{"setpid", Xsetpid,	"Set the identifier of a given table entry"},
	{"edit",   Xedit,	"Edit given table entry"},
	{"flags",  Xflags,	"Edit given table entry flags"},
	{"swap",   Xswap,	"Swap two partition entries"},
	{"print",  Xprint,	"Print loaded GPT partition table"},
	{"write",  Xwrite,	"Write loaded GPT to disk"},
	{"exit",   Xexit,	"Exit edit of current GPT, without saving changes"},
	{"quit",   Xquit,	"Quit edit of current GPT, saving current changes"},
	{"abort",  Xabort,	"Abort program without saving current changes"},
	{NULL,     NULL,	NULL}
};


int
USER_init(disk_t *disk, gpt_t *tt)
{
	if (GPT_init(disk, tt) == -ENOMEM)
		return (-ENOMEM);

	if (ask_yn("Do you wish to write new GUID partition table?"))
		Xwrite(NULL, disk, tt, NULL, 0);

	return (0);
}

int modified;

int
USER_modify(disk_t *disk, gpt_t *tt, off_t offset, off_t reloff)
{
	gpt_t gpt;
	cmd_t cmd;
	int i, st, fd, error;

	/* Set up command table pointer */
	cmd.table = cmd_table;

	/* Read GPT & partition */
	fd = DISK_open(disk->name, O_RDONLY);
	error = GPT_load(fd, disk, &gpt, 0);
	if (error == -1) {
		printf("Master header not clean, trying backup header\n");
		error = GPT_load(fd, disk, &gpt, 1);
	}
	close(fd);
	if (error == -1) {
		printf("Backup header not clean, exiting.\n");
		goto done;
	}

	printf("Enter 'help' for information\n");

	/* Edit cycle */
	do {
again:
		printf("gdisk:%c> ", (modified)?'*':' ');
		fflush(stdout);
		ask_cmd(&cmd);

		if (cmd.cmd[0] == '\0')
			goto again;
		for (i = 0; cmd_table[i].cmd != NULL; i++)
			if (strstr(cmd_table[i].cmd, cmd.cmd)==cmd_table[i].cmd)
				break;

		/* Quick hack to put in '?' == 'help' */
		if (!strcmp(cmd.cmd, "?"))
			i = 0;

		/* Check for valid command */
		if (cmd_table[i].cmd == NULL) {
			printf("Invalid command '%s'.  Try 'help'.\n", cmd.cmd);
			continue;
		} else
			strlcpy(cmd.cmd, cmd_table[i].cmd, sizeof cmd.cmd);

		/* Call function */
		st = cmd_table[i].fcn(&cmd, disk, &gpt, tt, offset);

		/* Update status */
		if (st == CMD_EXIT)
			break;
		if (st == CMD_SAVE)
			break;
		if (st == CMD_CLEAN)
			modified = 0;
		if (st == CMD_DIRTY)
			modified = 1;
	} while (1);

	/* Write out GPT */
	if (modified) {
		if (st == CMD_SAVE) {
			if (Xwrite(NULL, disk, &gpt, NULL, offset) == CMD_CONT)
				goto again;
			close(fd);
		} else
			printf("Aborting changes to current GPT.\n");
	}

	GPT_free(&gpt);

done:
	return (0);
}

int
USER_print_disk(disk_t *disk)
{
	off_t offset, firstoff;
	int fd, error;
	gpt_t gpt;

	fd = DISK_open(disk->name, O_RDONLY);
	offset = firstoff = 1;

	DISK_printmetrics(disk, NULL);

	error = GPT_load(fd, disk, &gpt, 0);
	if (error == -1) {
		printf("Master header not clean, trying backup header\n");
		error = GPT_load(fd, disk, &gpt, 1);
	}
	if (error == -1) {
		printf("Backup header not clean, exiting.\n");
		return (close(fd));
	}

	printf("Offset: %lld\t", offset);
	GPT_print(&gpt, NULL);

	GPT_free(&gpt);

	return (close(fd));
}

