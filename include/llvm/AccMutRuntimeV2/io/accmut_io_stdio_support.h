//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_STDIO_SUPPORT_H
#define LLVM_ACCMUT_IO_STDIO_SUPPORT_H

/* Special operations on file descriptors to support stdio (without synchronization) */
char *__accmutv2__fdgets__nosync(int fd, char *s, int size);

int __accmutv2__fdputs__nosync(int fd, const char *s);

int __accmutv2__fdeof__nosync(int fd);

#endif //LLVM_ACCMUT_IO_STDIO_SUPPORT_H
