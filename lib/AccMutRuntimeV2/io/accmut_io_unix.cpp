//
// Created by lusirui on 19-3-11.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h>
#include <llvm/AccMutRuntimeV2/accmut_io.h>
#include <algorithm>

static char buf_ori[1000000];

void check_all() {
    for (auto &fd : opened_file_list) {
        if (isatty(fd)) {
            fprintf(stderr, "Skip %d, is tty device\n", fd);
            continue;
        }
        struct stat sb;
        if (fstat(fd, &sb) == -1)
            return;
        auto buf = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!buf)
            return;
        fprintf(stderr, "Checking %d\n", fd);
        check_eq(fdmap[fd]->tag, file_descriptor::REAL_FILE);
        auto rfd = static_cast<real_file_descriptor *>(fdmap[fd]);
        check_mem(rfd->buffer.data(), buf, sb.st_size);
    }
}

void dump_text(int fd) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (fdstruct->tag != file_descriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<real_file_descriptor *>(fdstruct);
    write(STDERR_FILENO, rfd->buffer.data(), rfd->size);
}

void dump_bin(int fd) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (fdstruct->tag != file_descriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<real_file_descriptor *>(fdstruct);
    for (size_t i = 0; i < rfd->size; ++i) {
        fprintf(stderr, "%02x ", rfd->buffer[i]);
        if (rfd->buffer[i] == '\n')
            fprintf(stderr, "\n");
    }
}

extern "C" {

int __accmutv2__register(int fd, int flags) {
    fdmap[fd] = new real_file_descriptor(fd, flags);
    opened_file_list.push_back(fd);
    // TODO: check _fdmap[fd] available
    return 0;
}
int __accmutv2__deregister(int fd) {
    if (!fdmap[fd]) {
        errno = EBADF;
        return -1;
    }
    delete fdmap[fd];
    fdmap[fd] = nullptr;
    auto it = std::find(opened_file_list.begin(), opened_file_list.end(), fd);
    check_assert(it != opened_file_list.end(), fprintf(stderr, "Deregister %d not found\n", fd));
    opened_file_list.erase(it);
    return 0;
}

off_t __accmutv2__lseek__nosync(int fd, off_t offset, int whence) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    return fdstruct->lseek(offset, whence);
}

ssize_t __accmutv2__read__nosync(int fd, void *buf, size_t count) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    return fdstruct->read(buf, count);
}

ssize_t __accmutv2__write__nosync(int fd, const void *buf, size_t count) {
    file_descriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    return fdstruct->write(buf, count);
}

int __accmutv2__open(const char *pathname, int flags, ...) {
    int fd;
    if (flags & O_CREAT) {
        va_list lst;
        va_start(lst, flags);
        mode_t mode = va_arg(lst, mode_t);
        va_end(lst);
        if (MUTATION_ID == 0) {
            fd = open(pathname, flags, mode);
        } else {
            if (flags & O_TRUNC) {
                fd = open("/tmp/accmut_empty", flags, mode);
            } else {
                if (access(pathname, F_OK))
                    fd = open(pathname, flags, mode);
                else
                    fd = open("/tmp/accmut_empty", flags, mode);
            }
        }
    } else {
        if (MUTATION_ID == 0) {
            fd = open(pathname, flags);
        } else {
            if (flags & O_TRUNC) {
                fd = open("/tmp/accmut_empty", flags);
            } else {
                fd = open(pathname, flags);
            }
        }
    }
    // TODO: write and reopen is not handled correctly
    if (fd == -1)
        return -1;
    __accmutv2__register(fd, flags);
    if (MUTATION_ID != 0)
        close(fd);
    return fd;
}

int __accmutv2__creat(const char *pathname, mode_t mode) {
    return __accmutv2__open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int __accmutv2__close(int fd) {
    return __accmutv2__deregister(fd) | close(fd);
}

off_t __accmutv2__lseek(int fd, off_t offset, int whence) {
    off_t ori_ret = only_origin(::lseek(fd, offset, whence));
    off_t ret = __accmutv2__lseek__nosync(fd, offset, whence);
    check_eq(ori_ret, ret);
    return ret;
}

ssize_t __accmutv2__read(int fd, void *buf, size_t count) {
    // on stdin, this is broken
    ssize_t ori_ret = only_origin(::read(fd, buf_ori, count));
    ssize_t ret = __accmutv2__read__nosync(fd, buf, count);
    check_eq(ori_ret, ret);
    check_mem(buf, buf_ori, ret);
    return ret;
}

ssize_t __accmutv2__write(int fd, const void *buf, size_t count) {
    ssize_t ori_ret = only_origin(::write(fd, buf, count));
    ssize_t ret = __accmutv2__write__nosync(fd, buf, count);
    check_eq(ori_ret, ret);
    return ret;
}

void __accmutv2__checkfd_func() {
    check_all();
}

void __accmutv2__dump__text(int fd) {
    dump_text(fd);
}

void __accmutv2__dump__bin(int fd) {
    dump_bin(fd);
}

}