/*	$OpenBSD: stack_protector.c,v 1.20 2015/11/25 00:16:40 deraadt Exp $	*/

/*
 * Copyright (c) 2002 Hiroaki Etoh, Federico G. Schwindt, and Miodrag Vallat.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

long __guard[8] __attribute__((section(".openbsd.randomdata")));
/* gcc requires extern here and clang falls over with it */
#if !defined(__clang__) || (!(__clang_major__ == 3 && __clang_minor__ < 2))
extern
#endif
long __stack_chk_guard[8] __attribute__((alias("__guard")));

void __stack_smash_handler(const char func[], int damaged __attribute__((unused)));
void __attribute__((noreturn)) __stack_chk_fail(void);

/*ARGSUSED*/
void
__stack_smash_handler(const char func[], int damaged)
{
	extern char *__progname;
	struct sigaction sa;
	sigset_t mask;
	char buf[1024];
	size_t len;

	/* clang can't tell us the function */
	if (func == NULL) {
		message = "stack overflow detected; terminated";
		func = "";
	}

	/* Immediately block all signal handlers from running code */
	sigfillset(&mask);
	sigdelset(&mask, SIGABRT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	/* <10> is LOG_CRIT */
	len = strlcpy(buf, "<10>", sizeof buf);
	strlcpy(buf + len, __progname, sizeof buf - len);

	/* truncate progname in case it is too long */
	buf[sizeof(buf) / 2] = '\0';
	strlcat(buf, ": stack overflow in function ", sizeof buf);
	strlcat(buf, func, sizeof buf);

	sendsyslog2(buf, strlen(buf), LOG_CONS);

	bzero(&sa, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sa, NULL);

	kill(getpid(), SIGABRT);

	_exit(127);
}

DEF_WEAK(__stack_smash_handler);

void
__stack_chk_fail(void)
{
	__stack_smash_handler(NULL, 0);
	/* NOTREACHED */
	_exit(127);
}
