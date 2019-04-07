//
// Created by Sirui Lu on 2019-03-22.
//

#ifndef LLVM_ACCMUT_FS_SYSCALL_H
#define LLVM_ACCMUT_FS_SYSCALL_H

#include <unistd.h>

int __accmutv2__rename(const char *old, const char *newname);

int __accmutv2__remove(const char *path);

int __accmutv2__unlink(const char *path);

int __accmutv2__link(const char *path1, const char *path2);

int __accmutv2__dup(int fildes);

int __accmutv2__access(const char *path, int mode);

/* no implementation */
int __accmutv2__fsync(int fildes);

int __accmutv2__fchown(int fildes, uid_t owner, gid_t group);

int __accmutv2__chmod(const char *path, mode_t mode);

#endif //LLVM_ACCMUT_FS_SYSCALL_H
