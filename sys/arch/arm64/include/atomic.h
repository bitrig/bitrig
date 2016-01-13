/* Public Domain */

#ifndef _MACHINE_ATOMIC_H_
#define _MACHINE_ATOMIC_H_

#if defined(_KERNEL)
#include <sys/stdatomic.h>

static inline void
atomic_setbits_int(__volatile unsigned int *ptr, unsigned int val)
{
	atomic_fetch_or_explicit((atomic_uint *)ptr, val, memory_order_seq_cst);
}
static inline void
atomic_clearbits_int(__volatile unsigned int *ptr, unsigned int val)
{
	atomic_fetch_and_explicit((atomic_uint *)ptr, ~val, memory_order_seq_cst);
}
#endif /* defined(_KERNEL) */
#endif /* _MACHINE_ATOMIC_H_ */
