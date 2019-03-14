//
// Created by Sirui Lu on 2019-03-14.
//

#ifndef LLVM_TESTINIT_H
#define LLVM_TESTINIT_H

#include <stdio.h>
#include <type_traits>
#include <unistd.h>
#include <stdlib.h>

#define ACCMUT_START_EARLY

#include <llvm/AccMutRuntimeV2/accmut_config.h>
#include <llvm/AccMutRuntimeV2/accmut_io.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_check.h>
#include "testbase.h"

inline void print_ori() {
    printf("Ori\n");
};

inline void print_new() {
    printf("New\n");
}

DEFINE_FUNC(_print, print_ori, print_new);
DEFINE_FUNC(_open, open, __accmutv2__open);
DEFINE_FUNC(_creat, creat, __accmutv2__creat);
DEFINE_FUNC(_lseek, lseek, __accmutv2__lseek);
DEFINE_FUNC(_close, close, __accmutv2__close);
DEFINE_FUNC(_read, read, __accmutv2__read);
DEFINE_FUNC(_write, write, __accmutv2__write);
DEFINE_FUNC(_fopen, fopen, __accmutv2__fopen);
DEFINE_FUNC(_fclose, fclose, __accmutv2__fclose);
DEFINE_FUNC(_fputs, fputs, __accmutv2__fputs);

#define ALL_ADDED()\
ADD_FUNC(_print, print_ori);\
ADD_FUNC(_open, open);\
ADD_FUNC(_creat, creat);\
ADD_FUNC(_lseek, lseek);\
ADD_FUNC(_close, close);\
ADD_FUNC(_read, read);\
ADD_FUNC(_write, write);\
ADD_FUNC(_fopen, fopen);\
ADD_FUNC(_fclose, fclose);\
ADD_FUNC(_fputs, fputs);

#include "testdef.h"

#define RUN(...) __VA_ARGS__
#endif //LLVM_TESTINIT_H
