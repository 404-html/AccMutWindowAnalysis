//
// Created by lusirui on 19-3-10.
//

#include <llvm/AccMutRuntimeV2/accmut_config.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_buf_ori.h>

off_t real_file_descriptor::lseek(off_t offset, int whence) {
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

ssize_t real_file_descriptor::read(void *buf, size_t count) {
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

ssize_t real_file_descriptor::write(const void *buf, size_t count) {
    if ((flags & O_ACCMODE) == O_RDONLY) {
        errno = EBADF;
        return -1;
    }
    if (pos + count < size) {
        memcpy(buffer.data() + pos, buf, count);
        pos += count;
        return count;
    } else {
        buffer.resize(std::max<size_t>(buffer.size() * 2, pos + count));
        size = pos + count;
        memcpy(buffer.data() + pos, buf, count);
        pos += count;
        return count;
    }
}

char *real_file_descriptor::gets(char *s, int size, int &eof_seen) {
    if (size <= 0)
        return nullptr;
    if (size == 1) {
        s[0] = 0;
        return s;
    }
    if (pos >= this->size) {
        s[0] = 0;
        eof_seen = true;
        return nullptr;
    }
    int len = std::min(size - 1, (int) (this->size - pos)); // len >= 1
    char *t = (char *) memchr(buffer.data() + pos, '\n', len);

    if (t != nullptr) {
        len = (int) (t - buffer.data() - pos);
        ++t;
        ++len;
    } else {
        if (len != size - 1)
            eof_seen = true;
    }
    memcpy(s, buffer.data() + pos, len);
    pos += len;
    *(s + len) = 0;
    return s;
}

int real_file_descriptor::puts(const char *s) {
    size_t len = strlen(s);
    return (int) write(s, len);
}

int real_file_descriptor::eof() {
    if (pos >= size)
        return 1;
    return 0;
}

size_t real_file_descriptor::readobj(void *buf, size_t size, size_t nitem, int ungotchar) {
    size_t request = size * nitem;
    if (request == 0)
        return 0;
    if (ungotchar == EOF) {
        if (pos + request > this->size) {
            memcpy(buf, buffer.data() + pos, size - pos);
            size_t res = (this->size - pos) / size;
            pos = this->size;
            return res;
        } else {
            memcpy(buf, buffer.data() + pos, request);
            pos += request;
            return (int) nitem;
        }
    } else {
        ((char *) buf)[0] = (char) ungotchar;
        if (pos + request > this->size + 1) {
            memcpy((char *) buf + 1, buffer.data() + pos, size - pos);
            size_t res = (this->size - pos + 1) / size;
            pos = this->size;
            return res;
        } else {
            memcpy((char *) buf + 1, buffer.data() + pos, request - 1);
            pos += request - 1;
            return (int) nitem;
        }
    }
}

int real_file_descriptor::getc() {
    if (pos >= size)
        return EOF;
    int ret = buffer[pos];
    pos++;
    return ret;
}

int real_file_descriptor::printf(const char *format, va_list ap) {
    int ret = vsprintf(buf_ori, format, ap);
    if (ret < 0)
        return ret;
    write(buf_ori, (size_t) ret);
    return ret;
}

bool real_file_descriptor::canread() {
    return (flags & O_ACCMODE) != O_WRONLY;
}

bool real_file_descriptor::canwrite() {
    return (flags & O_ACCMODE) != O_RDONLY;
}

real_file_descriptor::real_file_descriptor(int fd, int flags) : file_descriptor(fd, REAL_FILE) {
    this->flags = flags;
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        return;
    auto buf = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!buf)
        return;
    buffer.resize(sb.st_size + 10);
    memcpy(buffer.data(), buf, sb.st_size);
    munmap(buf, sb.st_size);
    size = sb.st_size;
    if (flags & O_APPEND)
        pos = size;
    else
        pos = 0;
    ok = true;
}

off_t std_file_descriptor::lseek(off_t offset, int whence) {
    errno = EPIPE;
    return -1;
}

ssize_t stdin_file_descriptor::write(const void *buf, size_t count) {
    errno = EBADF;
    return -1;
}

ssize_t stdout_file_descriptor::write(const void *buf, size_t count) {
    return count;
}

int stdout_file_descriptor::puts(const char *s) {
    return 1;
}

int stdout_file_descriptor::printf(const char *format, va_list ap) {
    return vsprintf(buf_ori, format, ap);
}

