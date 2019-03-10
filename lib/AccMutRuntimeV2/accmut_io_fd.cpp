//
// Created by lusirui on 19-3-10.
//

#include <llvm/AccMutRuntimeV2/accmut_config.h>
#include <llvm/AccMutRuntimeV2/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/accmut_io.h>
#include <llvm/AccMutRuntimeV2/accmut_io_check.h>
#include <list>
#include <algorithm>

static char buf_ori[1000000];

off_t RealFileDescriptor::lseek(off_t offset, int whence) {
    off_t newpos;
    if (whence == SEEK_CUR) {
        newpos = pos + offset;
    } else if (whence == SEEK_END) {
        newpos = size + offset;
    } else if (whence == SEEK_SET) {
        newpos = offset;
    } else {
        errno = EINVAL;
        return -1;
    }
    if (newpos < 0) {
        errno = EINVAL;
        return -1;
    }
    pos = newpos;
    return pos;
}

off_t RealFileDescriptor::read(void *buf, size_t count) {
    if (flags & O_WRONLY) {
        errno = EBADF;
        return -1;
    }
    if (pos + count < size) {
        memcpy(buf, buffer.data() + pos, count);
        pos += count;
        return count;
    } else {
        auto len = size - pos;
        if (len == 0) {
            return 0;
        }
        memcpy(buf, buffer.data() + pos, len);
        pos = size;
        return len;
    }
}

ssize_t RealFileDescriptor::write(const void *buf, size_t count) {
    if (flags & O_RDONLY) {
        errno = EBADF;
        return -1;
    }
    if (pos + count < size) {
        memcpy(buffer.data() + pos, buf, count);
        pos += count;
        return count;
    } else {
        buffer.resize(std::max(buffer.size() * 2, pos + count));
        size = pos + count;
        memcpy(buffer.data() + pos, buf, count);
        pos += count;
        return count;
    }
}

RealFileDescriptor::RealFileDescriptor(int fd, int flags) : FileDescriptor(fd, REAL_FILE) {
    this->flags = flags;
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        return;
    auto buf = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!buf)
        return;
    buffer.resize(sb.st_size);
    memcpy(buffer.data(), buf, sb.st_size);
    size = sb.st_size;
    if (flags & O_APPEND)
        pos = size;
    else
        pos = 0;
    ok = true;
}

off_t StdFileDescriptor::lseek(off_t offset, int whence) {
    errno = EPIPE;
    return -1;
}

ssize_t StdinFileDescriptor::read(void *buf, size_t count) {
    check_assert(false, fprintf(stderr, "\tShould not read on stdin\n"));
    return ::read(fd, buf, count);
}

ssize_t StdinFileDescriptor::write(const void *buf, size_t count) {
    errno = EBADF;
    return -1;
}

ssize_t StdoutFileDescriptor::read(void *buf, size_t count) {
    errno = EBADF;
    return -1;
}

ssize_t StdoutFileDescriptor::write(const void *buf, size_t count) {
    return count;
}

FileDescriptor *fdmap[65536];
std::list<int> openedFileList;


class _InitFDMap {
    static bool once;
public:
    _InitFDMap();
};

bool _InitFDMap::once = false;
_InitFDMap _init_fdmap;

_InitFDMap::_InitFDMap() {
    if (!once) {
        memset(fdmap, 0, sizeof(FileDescriptor *) * 65536);
        if (isatty(0))
            fdmap[0] = new StdinFileDescriptor(0);
        else
            fdmap[0] = new RealFileDescriptor(0, O_RDONLY);
        if (isatty(1))
            fdmap[1] = new StdoutFileDescriptor(1);
        else
            fdmap[1] = new RealFileDescriptor(1, O_WRONLY);
        if (isatty(2))
            fdmap[2] = new StdoutFileDescriptor(2);
        else
            fdmap[2] = new RealFileDescriptor(2, O_WRONLY);
        openedFileList = {0, 1, 2};
        once = true;
    }
}

void check_all() {
    for (auto &fd : openedFileList) {
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
        check_eq(fdmap[fd]->tag, FileDescriptor::REAL_FILE);
        auto rfd = static_cast<RealFileDescriptor *>(fdmap[fd]);
        check_mem(rfd->buffer.data(), buf, sb.st_size);
    }
}

void dump_text(int fd) {
    FileDescriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (fdstruct->tag != FileDescriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<RealFileDescriptor *>(fdstruct);
    write(STDERR_FILENO, rfd->buffer.data(), rfd->size);
}

void dump_bin(int fd) {
    FileDescriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (fdstruct->tag != FileDescriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<RealFileDescriptor *>(fdstruct);
    for (size_t i = 0; i < rfd->size; ++i) {
        fprintf(stderr, "%02x ", rfd->buffer[i]);
        if (rfd->buffer[i] == '\n')
            fprintf(stderr, "\n");
    }
}

extern "C" {

int __accmutv2__register(int fd, int flags) {
    fdmap[fd] = new RealFileDescriptor(fd, flags);
    openedFileList.push_back(fd);
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
    auto it = std::find(openedFileList.begin(), openedFileList.end(), fd);
    check_assert(it != openedFileList.end(), fprintf(stderr, "Deregister %d not found\n", fd));
    openedFileList.erase(it);
    return 0;
}

off_t __accmutv2__lseek__nosync(int fd, off_t offset, int whence) {
    FileDescriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    off_t ret = fdstruct->lseek(offset, whence);
    return ret;
}

ssize_t __accmutv2__read__nosync(int fd, void *buf, size_t count) {
    FileDescriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    ssize_t ret = fdstruct->read(buf, count);
    return ret;
}

ssize_t __accmutv2__write__nosync(int fd, const void *buf, size_t count) {
    FileDescriptor *fdstruct = fdmap[fd];
    if (!fdstruct) {
        errno = EBADF;
        return -1;
    }
    ssize_t ret = fdstruct->write(buf, count);
    return ret;
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
