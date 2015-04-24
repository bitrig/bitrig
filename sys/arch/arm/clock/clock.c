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
#include <sys/queue.h>
#include <machine/clock.h>
#include <machine/fdt.h>

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

struct clk_provider {
	SLIST_ENTRY(clk_provider)	 cp_entry;
	void				*cp_node;
	struct clk			*(*cp_cb)(void *, void *, int *, int);
	void				*cp_cbarg;
};

SLIST_HEAD(, clk_provider) cp_list = SLIST_HEAD_INITIALIZER(clk_provider);

void
clk_fdt_register_provider(void *node,
    struct clk *(*cb)(void *, void *, int *, int), void *cbarg)
{
	struct clk_provider *cp = malloc(sizeof(struct clk_provider),
	    M_DEVBUF, M_NOWAIT|M_ZERO);
	if (cp == NULL)
		panic("%s: cannot allocate memory\n", __func__);

	cp->cp_node = node;
	cp->cp_cb = cb;
	cp->cp_cbarg = cbarg;

	SLIST_INSERT_HEAD(&cp_list, cp, cp_entry);
}

static struct clk *
clk_fdt_get_by_clkspec(void *node, int *clocks, int nclock)
{
	struct clk_provider *cp;

	SLIST_FOREACH(cp, &cp_list, cp_entry) {
		if (cp->cp_node != node)
			continue;
		return cp->cp_cb(cp->cp_cbarg, node, clocks, nclock);
	}

	return NULL;
}

struct clk *
clk_fdt_get(void *node, int index)
{
	int idx, nclk;

	if (node == NULL)
		return NULL;

	nclk = fdt_node_property(node, "clocks", NULL) / sizeof(uint32_t);
	if (nclk <= 0 || index > nclk)
		return NULL;

	int clocks[nclk];
	if (fdt_node_property_ints(node, "clocks", clocks, nclk) != nclk)
		panic("%s: can't extract clocks, but they exist", __func__);

	for (idx = 0; idx < nclk; idx++, index--) {
		void *clkc = fdt_find_node_by_phandle(NULL, clocks[idx]);
		if (clkc == NULL) {
			printf("%s: can't find clock controller\n",
			    __func__);
			return NULL;
		}

		int cells;
		if (!fdt_node_property_int(clkc, "#clock-cells", &cells)) {
			printf("%s: can't find size of clock cells\n",
			    __func__);
			return NULL;
		}

		/* At this point we found the controller. */
		if (index == 0)
			return clk_fdt_get_by_clkspec(clkc, &clocks[idx],
			    cells);

		idx += cells;
	}

	return NULL;
}

struct clk *
clk_fdt_get_by_name(void *node, char *name)
{
	char *data;
	int index = 0, len, nlen = strlen(name);

	if (node == NULL)
		return NULL;

	if (!fdt_node_property(node, "clocks", NULL))
		return NULL;

	len = fdt_node_property(node, "clock-names", &data);
	while (len > 0 && len >= nlen + 1) {
		if (!strncmp(data, name, nlen))
			return clk_fdt_get(node, index);
		len -= (strlen(data) + 1);
		data += (strlen(data) + 1);
		index += 1;
	}

	return NULL;
}
