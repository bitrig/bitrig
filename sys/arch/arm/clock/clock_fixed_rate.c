/*
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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
#include <sys/malloc.h>
#include <machine/clock.h>

struct clk_fixed
{
	uint32_t val;
};

static uint32_t
clk_fixed_rate_get_rate(struct clk *clk)
{
	struct clk_fixed *fix = clk->data;
	return fix->val;
}

struct clk *
clk_fixed_rate(char *name, uint32_t val)
{
	struct clk *clk;
	struct clk_fixed *fix;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	fix = malloc(sizeof(struct clk_fixed), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (fix == NULL)
		goto err;

	fix->val = val;
	clk->data = fix;
	clk->get_rate = clk_fixed_rate_get_rate;
	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}
