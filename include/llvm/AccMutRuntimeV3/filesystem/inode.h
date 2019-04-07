//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_INODE_H
#define LLVM_INODE_H

#include <sys/stat.h>
#include <string>
#include "BaseFile.h"

class inode {
    friend ino_t fs_cache(const char *str);
    struct stat meta;
    std::shared_ptr<BaseFile> content;
public:
    inode(const struct stat &meta, std::unique_ptr<BaseFile> content)
            : meta(meta), content(std::move(content)) {}
};

#endif //LLVM_INODE_H
