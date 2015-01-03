/*	$OpenBSD: v_redraw.c,v 1.6 2014/11/12 04:28:41 bentley Exp $	*/

/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#include "../common/common.h"
#include "../cl/cl.h"
#include "vi.h"

/*
 * v_redraw -- ^L, ^R
 *	Redraw the screen.
 */
int
v_redraw(SCR *sp, VICMD *vp)
{
	return (cl_refresh(sp, 1));
}
