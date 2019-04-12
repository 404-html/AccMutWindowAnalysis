//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_LINKING_H
#define LLVM_LINKING_H

#include <fcntl.h>
#include <sys/stat.h>
#include <string>

// no privilege checking

int path_resolve(const char *from, bool follow_symlink, std::string *to);

int delete_file(const char *name, bool follow_symlink);

/*
struct stat {
    mode_t st_mode;
    ino_t st_ino;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    off_t st_size;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    // other not implemented
};
*/


ino_t new_regular(const char *name, mode_t mode, const ino_t *ino = nullptr);

ino_t new_dir(const char *name, mode_t mode, ino_t *ino = nullptr);

ino_t new_symlink(const char *name, const char *linkto, ino_t *ino = nullptr);

#endif //LLVM_LINKING_H
