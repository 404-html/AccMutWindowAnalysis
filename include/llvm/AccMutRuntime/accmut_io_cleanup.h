#ifndef ACCMUT_IO_CLEANUP_H
#define ACCMUT_IO_CLEANUP_H

#include "llvm/AccMutRuntime/accmut_io.h"

#ifdef __cplusplus
extern "C" {
#endif

void __accmut__io__close__ori();

void __accmut__io__register(ACCMUT_FILE *ptr);

void __accmut__io__deregister(ACCMUT_FILE *ptr);

#ifdef __cplusplus
}
#endif

#endif
