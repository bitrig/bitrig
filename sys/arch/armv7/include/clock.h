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

#ifndef _MACHINE_CLOCK_H
struct clk {
	char *			name;
	int			users;
	struct clk *		parent;
	struct clk *		secondary;
	void *			data;
	int			(*enable) (struct clk *);
	void			(*disable) (struct clk *);
	uint32_t		(*get_rate) (struct clk *);
	void			(*set_rate) (struct clk *, uint32_t);
	uint32_t		(*round_rate) (struct clk *, uint32_t);
	int			(*set_parent) (struct clk *, struct clk *);
};

struct clk *			clk_get(const char *);
int				clk_register(struct clk *, char *);
int				clk_enable(struct clk *);
void				clk_disable(struct clk *);
uint32_t			clk_get_rate(struct clk *);
uint32_t			clk_get_rate_parent(struct clk *);
void				clk_set_rate(struct clk *, uint32_t);
uint32_t			clk_round_rate(struct clk *, uint32_t);
struct clk *			clk_get_parent(struct clk *);
int				clk_set_parent(struct clk *, struct clk *);

#ifdef CLK_DEBUG
void				clk_dump(void);
#endif
#endif /* !_MACHINE_CLOCK_H */
