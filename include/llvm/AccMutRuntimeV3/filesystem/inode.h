//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_INODE_H
#define LLVM_INODE_H

#include <sys/stat.h>
#include <string>
#include <queue>
#include "BaseFile.h"

class inode {
    friend ino_t fs_cache_helper(const char *path);

    friend void dump_helper(FILE *f, ino_t ino);
    struct stat meta;
    std::shared_ptr<BaseFile> content;
public:
    void dump(FILE *f);

    inode(const struct stat &meta, std::shared_ptr<BaseFile> content)
            : meta(meta), content(std::move(content)) {}
};

#endif //LLVM_INODE_H
