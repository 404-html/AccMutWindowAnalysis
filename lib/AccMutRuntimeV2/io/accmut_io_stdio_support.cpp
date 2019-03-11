//
// Created by lusirui on 19-3-11.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_stdio_support.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h>

char *__accmutv2__fdgets__nosync(int fd, char *s, int size) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return nullptr;
    }
    return fdstruct->gets(s, size);
}

int __accmutv2__fdputs__nosync(int fd, const char *s) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return EOF;
    }
    return fdstruct->puts(s);
}

int __accmutv2__fdeof__nosync(int fd) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    return fdstruct->eof();
}

