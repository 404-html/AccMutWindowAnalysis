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
#define DEFINE_ACCMUT_FUNC(name)\
    DEFINE_FUNC(_ ## name, name, __accmutv2__ ## name)
DEFINE_ACCMUT_FUNC(open);
DEFINE_ACCMUT_FUNC(creat);
DEFINE_ACCMUT_FUNC(lseek);
DEFINE_ACCMUT_FUNC(close);
DEFINE_ACCMUT_FUNC(read);
DEFINE_ACCMUT_FUNC(write);
DEFINE_ACCMUT_FUNC(fopen);
DEFINE_ACCMUT_FUNC(fclose);
DEFINE_ACCMUT_FUNC(freopen);
DEFINE_ACCMUT_FUNC(feof);
DEFINE_ACCMUT_FUNC(fileno);
DEFINE_ACCMUT_FUNC(fflush);
DEFINE_ACCMUT_FUNC(ferror);
DEFINE_ACCMUT_FUNC(fseek);
DEFINE_ACCMUT_FUNC(ftell);
DEFINE_ACCMUT_FUNC(fseeko);
DEFINE_ACCMUT_FUNC(ftello);
DEFINE_ACCMUT_FUNC(rewind);
DEFINE_ACCMUT_FUNC(fread);
DEFINE_ACCMUT_FUNC(fgets);
DEFINE_ACCMUT_FUNC(fgetc);
DEFINE_ACCMUT_FUNC(getc);
DEFINE_ACCMUT_FUNC(ungetc);
DEFINE_ACCMUT_FUNC(vfscanf);
DEFINE_ACCMUT_FUNC(fscanf);
DEFINE_ACCMUT_FUNC(scanf);
DEFINE_ACCMUT_FUNC(fputs);
DEFINE_ACCMUT_FUNC(fwrite);
DEFINE_ACCMUT_FUNC(vfprintf);
DEFINE_ACCMUT_FUNC(fprintf);
DEFINE_ACCMUT_FUNC(printf);
DEFINE_ACCMUT_FUNC(perror);
DEFINE_ACCMUT_FUNC(fputc);

#define ADD_ACCMUT_FUNC(name) ADD_FUNC(_ ## name, name)

#define ALL_ADDED()\
ADD_ACCMUT_FUNC(open);\
ADD_ACCMUT_FUNC(creat);\
ADD_ACCMUT_FUNC(lseek);\
ADD_ACCMUT_FUNC(close);\
ADD_ACCMUT_FUNC(read);\
ADD_ACCMUT_FUNC(write);\
ADD_ACCMUT_FUNC(fopen);\
ADD_ACCMUT_FUNC(fclose);\
ADD_ACCMUT_FUNC(freopen);\
ADD_ACCMUT_FUNC(feof);\
ADD_ACCMUT_FUNC(fileno);\
ADD_ACCMUT_FUNC(fflush);\
ADD_ACCMUT_FUNC(ferror);\
ADD_ACCMUT_FUNC(fseek);\
ADD_ACCMUT_FUNC(ftell);\
ADD_ACCMUT_FUNC(fseeko);\
ADD_ACCMUT_FUNC(ftello);\
ADD_ACCMUT_FUNC(rewind);\
ADD_ACCMUT_FUNC(fread);\
ADD_ACCMUT_FUNC(fgets);\
ADD_ACCMUT_FUNC(fgetc);\
ADD_ACCMUT_FUNC(getc);\
ADD_ACCMUT_FUNC(ungetc);\
ADD_ACCMUT_FUNC(vfscanf);\
ADD_ACCMUT_FUNC(fscanf);\
ADD_ACCMUT_FUNC(scanf);\
ADD_ACCMUT_FUNC(fputs);\
ADD_ACCMUT_FUNC(fwrite);\
ADD_ACCMUT_FUNC(vfprintf);\
ADD_ACCMUT_FUNC(fprintf);\
ADD_ACCMUT_FUNC(printf);\
ADD_ACCMUT_FUNC(perror);\
ADD_ACCMUT_FUNC(fputc);

#include "testdef.h"

#define RUN(...) __VA_ARGS__
#endif //LLVM_TESTINIT_H
