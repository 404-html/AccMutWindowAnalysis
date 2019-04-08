//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_CACHING_H
#define LLVM_CACHING_H

#include "inode.h"

ino_t fs_cache(const char *str);

void dump_cache(FILE *f);

#endif //LLVM_CACHING_H
