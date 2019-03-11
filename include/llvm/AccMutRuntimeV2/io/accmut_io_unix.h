//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_UNIX_H
#define LLVM_ACCMUT_IO_UNIX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Unix IO without synchronization */
int __accmutv2__register(int fd, int flags);
int __accmutv2__deregister(int fd);
ssize_t __accmutv2__lseek__nosync(int fd, off_t offset, int whence);
ssize_t __accmutv2__read__nosync(int fd, void *buf, size_t count);
ssize_t __accmutv2__write__nosync(int fd, const void *buf, size_t count);

/* Unix IO with synchronization*/
int __accmutv2__open(const char *pathname, int flags, ...);
int __accmutv2__creat(const char *pathname, mode_t mode);
int __accmutv2__close(int fd);
ssize_t __accmutv2__lseek(int fd, off_t offset, int whence);
ssize_t __accmutv2__read(int fd, void *buf, size_t count);
ssize_t __accmutv2__write(int fd, const void *buf, size_t count);

/* Unix IO check */
void __accmutv2__checkfd_func();
void __accmutv2__dump__text(int fd);
void __accmutv2__dump__bin(int fd);

#ifdef __cplusplus
};
#endif

#define __accmutv2__checkfd() do {\
    if (MUTATION_ID == 0) {\
        fprintf(stderr, "Check fd at %s:%s:%d:\n", __FILE__, __FUNCTION__, __LINE__);\
        __accmutv2__checkfd_func();\
    }\
} while (0)

#endif //LLVM_ACCMUT_IO_UNIX_H
