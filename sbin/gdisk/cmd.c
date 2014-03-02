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
#include <sys/disklabel.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "disk.h"
#include "misc.h"
#include "user.h"
#include "part.h"
#include "cmd.h"

int
Xreinit(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	GPT_init(disk, gpt);

	/* Tell em we did something */
	printf("In memory copy is initialized to:\n");
	printf("Offset: %d\t", offset);
	GPT_print(gpt, cmd->args);
	printf("Use 'write' to update disk.\n");

	return (CMD_DIRTY);
}

/* ARGSUSED */
int
Xswap(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	const char *errstr;
	char *from, *to;
	int pf, pt, ret;
	gpt_partition_t pp;

	ret = CMD_CONT;

	to = cmd->args;
	from = strsep(&to, " \t");

	if (to == NULL) {
		printf("partition number is invalid\n");
		return (ret);
	}

	pf = (int)strtonum(from, 0, 127, &errstr);
	if (errstr) {
		printf("partition number is %s: %s\n", errstr, from);
		return (ret);
	}
	pt = (int)strtonum(to, 0, 127, &errstr);
	if (errstr) {
		printf("partition number is %s: %s\n", errstr, to);
		return (ret);
	}

	if (pt == pf) {
		printf("%d same partition as %d, doing nothing.\n", pt, pf);
		return (ret);
	}

	pp = gpt->part[pt];
	gpt->part[pt] = gpt->part[pf];
	gpt->part[pf] = pp;

	ret = CMD_DIRTY;
	return (ret);
}


/* ARGSUSED */
int
Xedit(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	const char *errstr;
	int i, pn, ret, new;
	gpt_partition_t *pp;
	char buf[36];

	pn = (int)strtonum(cmd->args, 0, 127, &errstr);
	if (errstr) {
		printf("partition number is %s: %s\n", errstr, cmd->args);
		return (CMD_CONT);
	}
	pp = &gpt->part[pn];

	new = uuid_is_nil(&pp->type, NULL);

	/* Edit partition type */
	ret = Xsetpid(cmd, disk, gpt, tt, offset);

	/* Unused, so just zero out */
	if (uuid_is_nil(&pp->type, NULL)) {
		memset(pp, 0, sizeof(*pp));
		printf("Partition %d is disabled.\n", pn);
		return (ret);
	}

	/* Use remaining free space for new partitions */
	if (new)
		GPT_fillremaining(gpt, pn);

	memset(buf, 0, sizeof(buf));
	ask_string("Partition name", buf, sizeof(buf));
	for (i = 0; i < sizeof(buf); i++)
		pp->name[i] = buf[i];

	/* XXX: CMD_DIRTY */
	/* Change table entry */
	pp->start_lba = getuint(disk, "Partition offset", pp->start_lba,
	    gpt->header->end_lba);
	pp->end_lba = getuint(disk, "Partition size",
	    pp->end_lba - pp->start_lba + 1,
	    gpt->header->end_lba - pp->start_lba + 1) + pp->start_lba - 1;

	/* Catch zero size partition. */
	if (pp->start_lba > pp->end_lba) {
		memset(pp, 0, sizeof(*pp));
		printf("Partition %d is disabled.\n", pn);
		return (ret);
	}

	if (uuid_is_nil(&pp->guid, NULL))
		uuidgen(&pp->guid, 1);

	return (ret);
}

/* ARGSUSED */
int
Xsetpid(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	const char *errstr;
	int pn, dflt, num, ret;
	gpt_partition_t *pp;

	ret = CMD_CONT;

	pn = (int)strtonum(cmd->args, 0, 127, &errstr);
	if (errstr) {
		printf("partition number is %s: %s\n", errstr, cmd->args);
		return (ret);
	}
	pp = &gpt->part[pn];

	/* Print out current table entry */
	PRT_print(0, NULL, NULL);
	PRT_print(pn, pp, NULL);

	/* Ask for partition type */
	if (uuid_is_nil(&pp->type, NULL))
		dflt = GPTPTYP_OPENBSD;
	else
		dflt = PRT_pid_for_type(&pp->type);
	num = ask_pid(dflt);
	if (num != PRT_pid_for_type(&pp->type)) {
		PRT_set_type_by_pid(pp, num);
		ret = CMD_DIRTY;
	}

	return (ret);
}

/* ARGSUSED */
int
Xprint(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{

	DISK_printmetrics(disk, cmd->args);
	printf("Offset: %d\t", offset);
	GPT_print(gpt, cmd->args);

	return (CMD_CONT);
}

/* ARGSUSED */
int
Xwrite(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	int fd;

	if (GPT_verify(gpt))
		return (CMD_CONT);

	fd = DISK_open(disk->name, O_RDWR);

	printf("Writing GPT.\n");
	if (GPT_store(fd, disk, gpt) == -1) {
		int saved_errno = errno;
		warn("error writing GPT");
		close(fd);
		errno = saved_errno;
		return (CMD_CONT);
	}
	close(fd);

	return (CMD_CLEAN);
}

/* ARGSUSED */
int
Xquit(cmd, disk, r, tt, offset)
	cmd_t *cmd;
	disk_t *disk;
	gpt_t *r;
	gpt_t *tt;
	int offset;
{

	/* Nothing to do here */
	return (CMD_SAVE);
}

/* ARGSUSED */
int
Xabort(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	exit(0);

	/* NOTREACHED */
	return (CMD_CONT);
}


/* ARGSUSED */
int
Xexit(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{

	/* Nothing to do here */
	return (CMD_EXIT);
}

/* ARGSUSED */
int
Xhelp(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	cmd_table_t *cmd_table = cmd->table;
	int i;

	/* Hmm, print out cmd_table here... */
	for (i = 0; cmd_table[i].cmd != NULL; i++)
		printf("\t%s\t\t%s\n", cmd_table[i].cmd, cmd_table[i].help);
	return (CMD_CONT);
}

/* ARGSUSED */
int
Xflags(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	const char *errstr;
	int i, part = -1, active = 0, priority = 0, tries = 0, success = 0;
	char *partp, *activep, *priorityp, *triesp, *successp;

	activep = cmd->args;
	partp = strsep(&activep, " \t");
	priorityp = activep;
	activep = strsep(&priorityp, " \t");
	triesp = priorityp;
	priorityp = strsep(&triesp, " \t");
	successp = triesp;
	triesp = strsep(&successp, " \t");

	part = (int)strtonum(partp, 0, 127, &errstr);
	if (errstr) {
		printf("partition number is %s: %s.\n", errstr, partp);
		return (CMD_CONT);
	}

	if (activep == NULL && priorityp == NULL && triesp == NULL &&
	    successp == NULL) {
		if (ask_yn("Mark partition as DOS active?"))
			active = 1;

		priority = (gpt->part[part].attribute >> GPT_PRIORITY_SHIFT)
		    & GPT_PRIORITY_MASK;
		priority = getuint(disk, "Priority", priority,
		    GPT_PRIORITY_MAX);

		tries = (gpt->part[part].attribute >> GPT_TRIES_SHIFT)
		    & GPT_TRIES_MASK;
		tries = getuint(disk, "Tries", tries, GPT_TRIES_MAX);

		success = (gpt->part[part].attribute >> GPT_SUCCESS_SHIFT)
		    & GPT_SUCCESS_MASK;
		success = getuint(disk, "Success", success, GPT_SUCCESS_MAX);
	} else if (activep != NULL && priorityp != NULL && triesp != NULL &&
	    successp != NULL) {
		active = (int)strtonum(activep, 0, 1, &errstr);
		if (errstr) {
			printf("active is %s: %s.\n", errstr, activep);
			return (CMD_CONT);
		}

		priority = (int)strtonum(priorityp, 0, GPT_PRIORITY_MAX,
		    &errstr);
		if (errstr) {
			printf("priority is %s: %s.\n", errstr, priorityp);
			return (CMD_CONT);
		}

		tries = (int)strtonum(triesp, 0, GPT_TRIES_MAX, &errstr);
		if (errstr) {
			printf("tries is %s: %s.\n", errstr, triesp);
			return (CMD_CONT);
		}

		success = (int)strtonum(successp, 0, GPT_SUCCESS_MAX, &errstr);
		if (errstr) {
			printf("success is %s: %s.\n", errstr, successp);
			return (CMD_CONT);
		}
	} else {
		return (CMD_CONT);
	}

	if (active) {
		/* Set active flag */
		for (i = 0; i < gpt->header->partitions_num; i++) {
			if (i == part)
				gpt->part[i].attribute |= GPT_DOSACTIVE;
			else
				gpt->part[i].attribute &= ~GPT_DOSACTIVE;
		}
	}

	gpt->part[part].attribute &= ~(GPT_PRIORITY_MASK
	    << GPT_PRIORITY_SHIFT);
	gpt->part[part].attribute |= (priority & GPT_PRIORITY_MASK)
	    << GPT_PRIORITY_SHIFT;

	gpt->part[part].attribute &= ~(GPT_TRIES_MASK
	    << GPT_TRIES_SHIFT);
	gpt->part[part].attribute |= (tries & GPT_TRIES_MASK)
	    << GPT_TRIES_SHIFT;

	gpt->part[part].attribute &= ~(GPT_SUCCESS_MASK
	    << GPT_SUCCESS_SHIFT);
	gpt->part[part].attribute |= (success & GPT_SUCCESS_MASK)
	    << GPT_SUCCESS_SHIFT;

	return (CMD_DIRTY);
}

/* ARGSUSED */
int
Xmanual(cmd_t *cmd, disk_t *disk, gpt_t *gpt, gpt_t *tt, int offset)
{
	char *pager = "/usr/bin/less";
	char *p;
	sig_t opipe;
	extern const unsigned char manpage[];
	extern const int manpage_sz;
	FILE *f;

	opipe = signal(SIGPIPE, SIG_IGN);
	if ((p = getenv("PAGER")) != NULL && (*p != '\0'))
		pager = p;
	if (asprintf(&p, "gunzip -qc|%s", pager) != -1) {
		f = popen(p, "w");
		if (f) {
			(void) fwrite(manpage, manpage_sz, 1, f);
			pclose(f);
		}
		free(p);
	}

	(void)signal(SIGPIPE, opipe);
	return (CMD_CONT);
}

