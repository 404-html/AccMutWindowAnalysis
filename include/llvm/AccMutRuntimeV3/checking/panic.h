//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_PANIC_H
#define LLVM_PANIC_H

#include "check.h"
#include <stdio.h>

#define panic(_str) \
    do {\
        FILE *f = __check_fopen("/tmp/accmut_check.log", "a");\
        __check_fprintf(__check_stderr, "%s:%s:%d panic: %s\n", __FILE__, __FUNCTION__, __LINE__, _str);\
        __check_fprintf(f, "%s:%s:%d panic: %s\n", __FILE__, __FUNCTION__, __LINE__, _str);\
        __check_fclose(f);\
    } while(0)

#endif //LLVM_PANIC_H
