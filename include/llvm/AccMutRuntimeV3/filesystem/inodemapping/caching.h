//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_CACHING_H
#define LLVM_CACHING_H

#include "llvm/AccMutRuntimeV3/filesystem/datastructure/inode.h"
#include <map>

#define CHECK_R 0x4
#define CHECK_W 0x2
#define CHECK_X 0x1

// relative to real path, not in memory
ino_t cache_path(const char *str);

ino_t cache_tree(const char *str);

std::shared_ptr<inode> query_tree(const char *str, int checkmode);

std::shared_ptr<inode> ino2inode(ino_t);

#endif //LLVM_CACHING_H
