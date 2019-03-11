//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_FD_H
#define LLVM_ACCMUT_IO_FD_H

#include <vector>
#include <unistd.h>
#include <map>
#include "accmut_io_check.h"

class file_descriptor {
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

    virtual char *gets(char *s, int size) = 0;

    virtual int puts(const char *s) = 0;

    virtual int eof() = 0;

    virtual ~file_descriptor() = default;

    inline explicit file_descriptor(int fd, TAG tag) : tag(tag), fd(fd) {};
};

class real_file_descriptor : public file_descriptor {
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

    char *gets(char *s, int size) override;

    int puts(const char *s) override;

    int eof() override;

    real_file_descriptor(int fd, int flags);
};

class std_file_descriptor : public file_descriptor {
public:
    off_t lseek(off_t offset, int whence) override;

    inline char *gets(char *s, int size) override {
        check_assert(false, fprintf(stderr, "gets for std file not implemented"));
        return nullptr;
    }

    inline ssize_t read(void *buf, size_t count) override {
        check_assert(false, fprintf(stderr, "read for std file not implemented"));
        return -1;
    }

    inline int eof() override {
        check_assert(false, fprintf(stderr, "eof for std file not implemented"));
        return 0;
    }

    inline explicit std_file_descriptor(int fd, TAG tag) : file_descriptor(fd, tag) {
        ok = true;
    }
};

class stdin_file_descriptor : public std_file_descriptor {
public:
    ssize_t write(const void *buf, size_t count) override;

    inline int puts(const char *s) override {
        return EOF;
    }

    inline explicit stdin_file_descriptor(int fd) : std_file_descriptor(fd, STDIN_FILE) {};
};

class stdout_file_descriptor : public std_file_descriptor {
public:
    ssize_t write(const void *buf, size_t count) override;

    int puts(const char *s) override;

    inline explicit stdout_file_descriptor(int fd) : std_file_descriptor(fd, STDOUT_FILE) {};
};


#endif //LLVM_ACCMUT_IO_FD_H
