/*
 * Copyright (c) 2014 Pedro Martelletto <pedro@ambientworks.net>
 * Copyright (c) 2014 Martin Natano <natano@natano.net>
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

#include <sys/types.h>

#include <err.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ps.h"

struct element {
	struct element	*left;
	struct element	*right;
	struct element	*parent;
	struct element	*child;
	struct element	*sibling;
	pid_t		 key;
	int		 index;
	bool		 visible;
};

static struct element root = { NULL, NULL, NULL, NULL, NULL, -1, -1, false };

/* Unlink an element from its parent. */
static void
orphan(struct element *element)
{
	struct element *prev;

	if (element->parent->child == element)
		element->parent->child = NULL;
	else {
		prev = element->parent->child;
		while (prev->sibling != element)
			prev = prev->sibling;
		prev->sibling = element->sibling;
	}

	element->parent = NULL;
	element->sibling = NULL;
}

/* Establish a parent/child link between two elements in the PID tree. */
static void
link(struct element *child, struct element *parent)
{
	if (child->parent)
		orphan(child);

	child->parent = parent;

	if (parent->child == NULL)
		parent->child = child;
	else {
		struct element *element = parent->child;
		while (element->sibling != NULL)
			element = element->sibling;
		element->sibling = child;
	}
}

static struct element *
new(pid_t key)
{
	struct element *element;

	element = calloc(1, sizeof(*element));
	if (element == NULL)
		err(1, NULL);

	element->key = key;
	element->index = -1;

	return (element);
}

/* Fetch element from the PID tree by 'key'. Insert a new one when not found. */
static struct element *
lookup(struct element *element, pid_t key)
{
	if (element->key == key)
		return (element);
	if (key < element->key) {
		if (element->left != NULL)
			return (lookup(element->left, key));
		return ((element->left = new(key)));
	} else {
		if (element->right != NULL)
			return (lookup(element->right, key));
		return ((element->right = new(key)));
	}
}

/* Mark 'pid' as a child of 'ppid'. */
static void
mark(pid_t pid, pid_t ppid)
{
	struct element *child;
	struct element *parent;

	child = lookup(&root, pid);
	child->visible = true;

	/* The kernel swapper thread is its own parent. */
	if (pid == ppid)
		parent = &root;
	else {
		parent = lookup(&root, ppid);
		if (!parent->parent)
			link(parent, &root);
	}

	if (child->parent != parent)
		link(child, parent);
}

static void
enumerate(struct element *root)
{
	struct element *e;
	int i;

	for (i = 0, e = root; e; i++) {
		e->index = i;

		if (e->child) {
			e = e->child;
			continue;
		}
		while (e->parent && !e->sibling && e != root)
			e = e->parent;
		if (e == root)
			break;

		e = e->sibling;
	}
}

static int
compare(const void *v1, const void *v2)
{
	const struct kinfo_proc *kp1 = *(const struct kinfo_proc **)v1;
	const struct kinfo_proc *kp2 = *(const struct kinfo_proc **)v2;

	return (lookup(&root, kp1->p_pid)->index -
	   lookup(&root, kp2->p_pid)->index);
}

/*
 * Return the ASCII art prefix for a PID. The returned value points to a
 * static buffer internal to the function, which will change on the next
 * invocation.
 */
char *
hier_prefix(pid_t key)
{
	static char prefix[_POSIX2_LINE_MAX];
	struct element *element;
	char *p;

	element = lookup(&root, key);

	/*
	 * Construct the prefix string in reverse order at the end of the buffer,
	 * so the resulting string's length doesn't have to be known beforehand.
	 */

	p = &prefix[sizeof(prefix) - 1];
	*--p = '\0';

#define ISROOT(e)	((e)->visible && !(e)->parent->visible)

	if (!element->parent || ISROOT(element))
		return (p);
	memcpy((p -= 3), element->sibling ? "|- " : "`- ", 3);
	while (element->parent && !ISROOT(element->parent)) {
		p -= 3;
		if (p < prefix)
			errx(1, "process hierarchy too deep");
		element = element->parent;
		memcpy(p, element->sibling ? "|  " : "   ", 3);
	}

	return (p);
}

/* Initialize PID tree and perform stable hierarchical sorting. */
void
hier_sort(struct kinfo_proc **kinfo, int nentries)
{
	int i;

	for (i = 0; i < nentries; i++)
		mark(kinfo[i]->p_pid, kinfo[i]->p_ppid);

	enumerate(&root);
	qsort(kinfo, nentries, sizeof(*kinfo), compare);
}
