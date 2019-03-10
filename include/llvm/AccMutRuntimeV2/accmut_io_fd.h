//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_FD_H
#define LLVM_ACCMUT_IO_FD_H

#include <vector>
#include <unistd.h>
#include <map>

class FileDescriptor {
public:
    enum TAG {
        REAL_FILE,
        STDIN_FILE,
        STDOUT_FILE
    };
    const TAG tag;
protected:
    bool ok = false;
    int fd;
public:
    virtual off_t lseek(off_t offset, int whence) = 0;

    virtual ssize_t read(void *buf, size_t count) = 0;

    virtual ssize_t write(const void *buf, size_t count) = 0;

    virtual ~FileDescriptor() = default;

    inline explicit FileDescriptor(int fd, TAG tag) : tag(tag), fd(fd) {};
};

class RealFileDescriptor : public FileDescriptor {
    friend void check_all();

    friend void dump_text(int);

    friend void dump_bin(int);

    off_t pos;
    off_t size;
    int flags;
    std::vector<char> buffer;
public:
    off_t lseek(off_t offset, int whence) override;

    ssize_t read(void *buf, size_t count) override;

    ssize_t write(const void *buf, size_t count) override;

    RealFileDescriptor(int fd, int flags);
};

class StdFileDescriptor : public FileDescriptor {
public:
    off_t lseek(off_t offset, int whence) override;

    inline explicit StdFileDescriptor(int fd, TAG tag) : FileDescriptor(fd, tag) {
        ok = true;
    }
};

class StdinFileDescriptor : public StdFileDescriptor {
public:
    ssize_t read(void *buf, size_t count) override;

    ssize_t write(const void *buf, size_t count) override;

    inline explicit StdinFileDescriptor(int fd) : StdFileDescriptor(fd, STDIN_FILE) {};
};

class StdoutFileDescriptor : public StdFileDescriptor {
public:
    ssize_t read(void *buf, size_t count) override;

    ssize_t write(const void *buf, size_t count) override;

    inline explicit StdoutFileDescriptor(int fd) : StdFileDescriptor(fd, STDOUT_FILE) {};
};


#endif //LLVM_ACCMUT_IO_FD_H
