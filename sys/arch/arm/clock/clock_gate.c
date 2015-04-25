/*
 * Copyright (c) 2015 Patrick Wildt <patrick@blueri.se>
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

struct clk_gate {
	uint32_t reg;
	uint32_t val;
	uint32_t shift;
	uint32_t mask;
	struct clk_mem *mem;
};

/*
 * 1-bit wide clock gate
 */
struct clk *
clk_gate(char *name, struct clk *parent, uint32_t reg, uint32_t shift, struct clk_mem *mem)
{
	struct clk *clk;
	struct clk_gate *gate;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	gate = malloc(sizeof(struct clk_gate), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (gate == NULL)
		goto err;

	gate->reg = reg;
	gate->val = 1;
	gate->mask = 0x1;
	gate->shift = shift;
	gate->mem = mem;
	clk->data = gate;
	clk->enable = clk_gate_enable;
	clk->disable = clk_gate_disable;
	clk->get_rate = clk_get_rate_parent;
	clk->parent = parent;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

/*
 * 2-bit wide clock gate
 */
struct clk *
clk_gate2(char *name, struct clk *parent, uint32_t reg, uint32_t shift, struct clk_mem *mem)
{
	struct clk *clk = clk_gate(name, parent, reg, shift, mem);
	struct clk_gate *gate;

	if (clk == NULL)
		return NULL;

	gate = clk->data;
	gate->val = 3;
	gate->mask = 0x3;

	return clk;
}

int
clk_gate_enable(struct clk *clk)
{
	struct clk_gate *gate = clk->data;

	bus_space_write_4(gate->mem->iot, gate->mem->ioh, gate->reg,
	    (bus_space_read_4(gate->mem->iot, gate->mem->ioh, gate->reg)
	    & ~(gate->mask << gate->shift)) | (gate->val << gate->shift));
	return 0;
}

void
clk_gate_disable(struct clk *clk)
{
	struct clk_gate *gate = clk->data;

	bus_space_write_4(gate->mem->iot, gate->mem->ioh, gate->reg,
	    bus_space_read_4(gate->mem->iot, gate->mem->ioh, gate->reg)
	    & ~(gate->mask << gate->shift));
}
