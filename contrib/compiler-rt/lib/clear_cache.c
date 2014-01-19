/* ===-- clear_cache.c - Implement __clear_cache ---------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"


#if __APPLE__
  #include <libkern/OSCacheControl.h>
#endif

#if defined(__OpenBSD__) || defined(__Bitrig__)
# if defined(__arm__) 
#include <machine/asm.h>
#include <sys/syscall.h>
#include <arm/swi.h>

static inline void
sysarch (int32_t sysarch_id, void *arg) {
	__asm volatile(" mov r0, %0;"
	    " mov r1, %1;"
	    " ldr r12, =%2;"
	    " swi %3" ::
	    "r" (sysarch_id), "r" (arg), "i"(SYS_sysarch), "i"(SYS_sysarch|SWI_OS_NETBSD):
	    "0", "1");
								    
}
# endif
#endif

/*
 * The compiler generates calls to __clear_cache() when creating 
 * trampoline functions on the stack for use with nested functions.
 * It is expected to invalidate the instruction cache for the 
 * specified range.
 */

void __clear_cache(void* start, void* end)
{
#if defined(__i386__) || defined(__x86_64__)
/*
 * Intel processors have a unified instruction and data cache
 * so there is nothing to do
 */
#else
    #if __APPLE__
        /* On Darwin, sys_icache_invalidate() provides this functionality */
        sys_icache_invalidate(start, end-start);
    #elif (defined(__OpenBSD__) || defined(__Bitrig__)) && defined(__arm__)
    	struct {
		unsigned int addr;
		int          len;
	} s;
	s.addr = (unsigned int)(start);
	s.len = (end) - (start);
	(void) sysarch (0, &s);   
    #else
        compilerrt_abort();
    #endif
#endif
}

