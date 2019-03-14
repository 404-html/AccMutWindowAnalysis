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

size_t __accmutv2__fdread__nosync(int fd, void *buf, size_t size, size_t nitem, int ungotchar) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return 0;
    }
    return fdstruct->readobj(buf, size, nitem, ungotchar);
}

size_t __accmutv2__fdwrite__nosync(int fd, const void *buf, size_t size, size_t nitem) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return 0;
    }
    ssize_t ret = fdstruct->write(buf, size * nitem);
    if (ret < 0)
        return 0;
    return (size_t) ret;
}

int __accmutv2__fdgetc__nosync(int fd) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return 0;
    }
    return fdstruct->getc();
}

int __accmutv2__fdprintf__nosync(int fd, const char *format, va_list ap) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return 0;
    }
    return fdstruct->printf(format, ap);
}
