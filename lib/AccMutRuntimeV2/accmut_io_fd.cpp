//
// Created by lusirui on 19-3-10.
//

#include <llvm/AccMutRuntimeV2/accmut_config.h>
#include <llvm/AccMutRuntimeV2/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/accmut_io.h>
#include <llvm/AccMutRuntimeV2/accmut_io_check.h>


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

std::map<int, FileDescriptor *> _fdmap;


class _InitFDMap {
    static bool once;
public:
    _InitFDMap();
};

bool _InitFDMap::once = false;
_InitFDMap _init_fdmap;

_InitFDMap::_InitFDMap() {
    if (!once) {
        if (isatty(0))
            _fdmap[0] = new StdinFileDescriptor(0);
        else
            _fdmap[0] = new RealFileDescriptor(0, O_RDONLY);
        if (isatty(1))
            _fdmap[1] = new StdoutFileDescriptor(1);
        else
            _fdmap[1] = new RealFileDescriptor(1, O_WRONLY);
        if (isatty(2))
            _fdmap[2] = new StdoutFileDescriptor(2);
        else
            _fdmap[2] = new RealFileDescriptor(2, O_WRONLY);
        once = true;
    }
}

void check_all() {
    for (auto &t : _fdmap) {
        if (isatty(t.first)) {
            fprintf(stderr, "Skip %d, is tty device\n", t.first);
            continue;
        }
        struct stat sb;
        if (fstat(t.first, &sb) == -1)
            return;
        auto buf = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, t.first, 0);
        if (!buf)
            return;
        fprintf(stderr, "Checking %d\n", t.first);
        check_eq(t.second->tag, FileDescriptor::REAL_FILE);
        auto rfd = static_cast<RealFileDescriptor *>(t.second);
        check_mem(rfd->buffer.data(), buf, sb.st_size);
    }
}

void dump_text(int fd) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (iter->second->tag != FileDescriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<RealFileDescriptor *>(iter->second);
    write(STDERR_FILENO, rfd->buffer.data(), rfd->size);
}

void dump_bin(int fd) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        fprintf(stderr, "Don't know file %d\n", fd);
        return;
    }
    if (iter->second->tag != FileDescriptor::REAL_FILE) {
        fprintf(stderr, "File %d is not a regular file\n", fd);
        return;
    }
    fprintf(stderr, "Dump file %d\n", fd);
    auto rfd = static_cast<RealFileDescriptor *>(iter->second);
    for (size_t i = 0; i < rfd->size; ++i) {
        fprintf(stderr, "%02x ", rfd->buffer[i]);
        if (rfd->buffer[i] == '\n')
            fprintf(stderr, "\n");
    }
}

extern "C" {


int __accmutv2__open(const char *pathname, int flags, ...) {
    int fd;
    if (flags & O_CREAT) {
        va_list lst;
        va_start(lst, flags);
        mode_t mode = va_arg(lst, mode_t);
        va_end(lst);
        fd = open(pathname, flags, mode);
    } else {
        fd = open(pathname, flags);
    }
    if (fd == -1)
        return -1;
    _fdmap[fd] = new RealFileDescriptor(fd, flags);
    return fd;
}

int __accmutv2__creat(const char *pathname, mode_t mode) {
    return __accmutv2__open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int __accmutv2__close(int fd) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        errno = EBADF;
        return -1;
    }
    delete _fdmap[fd];
    _fdmap.erase(iter);
    return close(fd);
}

off_t __accmutv2__lseek(int fd, off_t offset, int whence) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        errno = EBADF;
        return -1;
    }
    off_t ori_ret = only_origin(::lseek(fd, offset, whence));
    off_t ret = iter->second->lseek(offset, whence);
    check_eq(ori_ret, ret);
    return ret;
}

ssize_t __accmutv2__read(int fd, void *buf, size_t count) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        errno = EBADF;
        return -1;
    }
    // on stdin, this is broken
    off_t ori_ret = only_origin(::read(fd, buf_ori, count));
    off_t ret = iter->second->read(buf, count);
    check_eq(ori_ret, ret);
    check_mem(buf, buf_ori, ret);
    return ret;
}

ssize_t __accmutv2__write(int fd, const void *buf, size_t count) {
    auto iter = _fdmap.find(fd);
    if (iter == _fdmap.end()) {
        errno = EBADF;
        return -1;
    }
    off_t ori_ret = only_origin(::write(fd, buf, count));
    off_t ret = iter->second->write(buf, count);
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
