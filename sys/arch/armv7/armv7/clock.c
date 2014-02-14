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

struct clk_list {
	struct clk *		clk;
	struct clk_list *	next;
};

static struct clk_list *	clk_root;

int
clk_enable(struct clk *clk)
{
	if (clk == NULL)
		return 1;

	/* Make sure we and the parent are enabled on first use. */
	if (clk->users++ == 0) {
		clk_enable(clk->parent);
		clk_enable(clk->secondary);

		if (clk->enable)
			clk->enable(clk);
	}

	return 0;
}

void
clk_disable(struct clk *clk)
{
	if (clk == NULL)
		return;

	/* Disable us and the parent if we now stop having any users. */
	if (--clk->users == 0) {
		if (clk->disable)
			clk->disable(clk);

		clk_disable(clk->parent);
		clk_disable(clk->secondary);
	}
}

uint32_t
clk_get_rate(struct clk *clk)
{
	if (clk == NULL || clk->get_rate == NULL)
		return 0;

	return clk->get_rate(clk);
}

uint32_t
clk_get_rate_parent(struct clk *clk)
{
	if (clk == NULL || clk->parent == NULL)
		return 0;

	return clk_get_rate(clk->parent);
}

void
clk_set_rate(struct clk *clk, uint32_t rate)
{
	if (clk == NULL || clk->set_rate == NULL)
		return;

	clk->set_rate(clk, rate);
}

uint32_t
clk_round_rate(struct clk *clk, uint32_t rate)
{
	if (clk == NULL || clk->round_rate == NULL)
		return 0;

	return clk->round_rate(clk, rate);
}

struct clk *
clk_get_parent(struct clk *clk)
{
	if (clk == NULL)
		return NULL;

	return clk->parent;
}

int
clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct clk *old = parent;

	if (clk == NULL || clk->set_parent == NULL || parent == NULL)
		return 1;

	/* Before we change our parent, make sure he's alive if we need him. */
	if (clk->users != 0)
		clk_enable(parent);

	if (!clk->set_parent(clk, parent)) {
		old = clk->parent;
		clk->parent = parent;
	}

	/* Disable the old clock if we previously used it. */
	if (clk->users != 0)
		clk_disable(old);

	/* If we didn't change the parent, let the caller know. */
	if (parent == old)
		return 1;

	return 0;
}

struct clk *
clk_get(const char *name)
{
	struct clk_list *root = clk_root;

	if (name == NULL || !strlen(name) || clk_root == NULL)
		return NULL;

	while (root != NULL) {
		if (!strcmp(root->clk->name, name))
			return root->clk;
		root = root->next;
	}

	return NULL;
}

int
clk_register(struct clk *clk, char *name)
{
	struct clk_list *node = NULL;
	struct clk_list *root = clk_root;

	if (clk == NULL || name == NULL || !strlen(name))
		return 1;

	if (clk_get(name) != NULL)
		return 1;

	node = malloc(sizeof(struct clk_list), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (node == NULL)
		return 1;

	clk->name = name;
	node->clk = clk;
	node->next = NULL;

	if (clk_root == NULL) {
		clk_root = node;
	} else {
		while (root->next != NULL)
			root = root->next;
		root->next = node;
	}

	return 0;
}

#ifdef CLK_DEBUG
void
clk_dump(void)
{
	struct clk_list *root = clk_root;

	while (root != NULL) {
		printf("%s: users: %d rate: %u\n", root->clk->name,
		    root->clk->users, clk_get_rate(root->clk));
		root = root->next;
	}
}
#endif
