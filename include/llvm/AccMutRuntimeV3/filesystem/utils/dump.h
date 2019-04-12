//
// Created by Sirui Lu on 2019-04-11.
//

#ifndef LLVM_DUMP_H
#define LLVM_DUMP_H

#include <stdio.h>

void dump_cache(FILE *f, const char *base = "/");

void dump_single(FILE *f, const char *file);

#endif //LLVM_DUMP_H
