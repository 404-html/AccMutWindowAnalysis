//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_CACHING_H
#define LLVM_CACHING_H

#include "llvm/AccMutRuntimeV3/filesystem/datastructure/inode.h"
#include <map>

extern std::map<ino_t, std::shared_ptr<inode>> inomap;

// relative to real path, not in memory
ino_t cache_path(const char *str);

ino_t cache_tree(const char *str);

std::shared_ptr<inode> query_tree(const char *str, mode_t mode);

#endif //LLVM_CACHING_H
