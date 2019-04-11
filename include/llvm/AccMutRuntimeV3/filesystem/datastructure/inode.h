//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_INODE_H
#define LLVM_INODE_H

#include <sys/stat.h>
#include <string>
#include <queue>
#include "BaseFile.h"
#include <llvm/AccMutRuntimeV3/filesystem/utils/perm.h>

class inode {
    friend void dump_helper(FILE *f, ino_t ino);
    struct stat meta;
    std::shared_ptr<BaseFile> content;
public:
    virtual ~inode() = default;
    void dump(FILE *f);

    // meta-data
    inline struct stat getStat() {
        return meta;
    }

    inline bool isDir() {
        return S_ISDIR(meta.st_mode);
    }

    inline bool isLnk() {
        return S_ISLNK(meta.st_mode);
    }

    inline bool isReg() {
        return S_ISREG(meta.st_mode);
    }

    inline bool canRead() {
        return check_read_perm(meta);
    }

    inline bool canWrite() {
        return check_write_perm(meta);
    }

    inline bool canExecute() {
        return check_execute_perm(meta);
    }

    inline ino_t getIno() {
        return meta.st_ino;
    }

    inline bool cached() {
        return (bool) content;
    }

    inline std::shared_ptr<BaseFile> getUnderlyingFile() {
        return content;
    }

    inline void incRef() { meta.st_nlink++; }

    inline void decRef() { meta.st_nlink--; }

    inline void setMod(mode_t mode) {
        meta.st_mode = (meta.st_mode & (mode_t) S_IFMT) | mode;
    }

    inline void setUid(uid_t uid) {
        meta.st_uid = uid;
    }

    inline void setGid(gid_t gid) {
        meta.st_gid = gid;
    }

    inline inode(const struct stat &meta, std::shared_ptr<BaseFile> content)
            : meta(meta), content(std::move(content)) {}
};

#endif //LLVM_INODE_H
