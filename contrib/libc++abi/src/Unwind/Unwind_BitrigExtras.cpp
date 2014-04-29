//===--------------------- Unwind_AppleExtras.cpp -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "DwarfParser.hpp"
#include "unwind_ext.h"

#if _LIBUNWIND_BUILD_SJLJ_APIS
#include <pthread.h>

static int pthreadKeyCreated;
static pthread_key_t Unwind_SjLj_Key;

// Accessors to get get/set linked list of frames for sjlj based execeptions.
_LIBUNWIND_HIDDEN
struct _Unwind_FunctionContext *__Unwind_SjLj_GetTopOfFunctionStack() {
  if (!pthreadKeyCreated) {
    pthread_key_create(&Unwind_SjLj_Key, NULL);
    pthreadKeyCreated = 1;
  }
  return (struct _Unwind_FunctionContext *)
    pthread_getspecific(Unwind_SjLj_Key);
}

_LIBUNWIND_HIDDEN
void __Unwind_SjLj_SetTopOfFunctionStack(struct _Unwind_FunctionContext *fc) {
  pthread_setspecific(Unwind_SjLj_Key, fc);
}
#endif
