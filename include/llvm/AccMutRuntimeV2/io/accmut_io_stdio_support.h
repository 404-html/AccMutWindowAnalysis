//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_STDIO_SUPPORT_H
#define LLVM_ACCMUT_IO_STDIO_SUPPORT_H

#include <stddef.h>
#include <stdarg.h>

/* Special operations on file descriptors to support stdio (without synchronization) */
char *__accmutv2__fdgets__nosync(int fd, char *s, int size, int &eof_seen);

int __accmutv2__fdputs__nosync(int fd, const char *s);

int __accmutv2__fdeof__nosync(int fd);

size_t __accmutv2__fdread__nosync(int fd, void *buf, size_t size, size_t nitem, int ungotchar);

size_t __accmutv2__fdwrite__nosync(int fd, const void *buf, size_t size, size_t nitem);

int __accmutv2__fdgetc__nosync(int fd);

int __accmutv2__fdprintf__nosync(int fd, const char *format, va_list ap);

int __accmutv2__fdputc__nosync(int fd, int c);

#endif //LLVM_ACCMUT_IO_STDIO_SUPPORT_H
