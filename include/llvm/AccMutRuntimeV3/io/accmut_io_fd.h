//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_FD_H
#define LLVM_ACCMUT_IO_FD_H

#include <vector>
#include <unistd.h>
#include <map>
#include "accmut_io_check.h"
#include "../fs/accmut_fs_memfile.h"
#include "../fs/accmut_fs_simulate.h"

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

    virtual char *gets(char *s, int size, int &eof_seen) = 0;

    virtual int puts(const char *s) = 0;

    virtual int eof() = 0;

    virtual size_t readobj(void *buf, size_t size, size_t nitem, int ungotchar) = 0;

    virtual int getc() = 0;

    virtual int printf(const char *format, va_list ap) = 0;

    virtual bool canread() = 0;

    virtual bool canwrite() = 0;

    virtual int putc(int c) = 0;

    virtual ~file_descriptor() = default;

    inline explicit file_descriptor(int fd, TAG tag) : tag(tag), fd(fd) {};
};

class real_file_descriptor : public file_descriptor {
    friend void check_all();

    friend void dump_text(int);

    friend void dump_bin(int);

    off_t pos;
    int flags;
    // std::shared_ptr<std::vector<char>> buffer;
    std::shared_ptr<mem_file> file;
public:
    off_t lseek(off_t offset, int whence) override;

    ssize_t read(void *buf, size_t count) override;

    ssize_t write(const void *buf, size_t count) override;

    char *gets(char *s, int size, int &eof_seen) override;

    int puts(const char *s) override;

    int eof() override;

    size_t readobj(void *buf, size_t size, size_t nitem, int ungotchar) override;

    int getc() override;

    int printf(const char *format, va_list ap) override;

    bool canread() override;

    bool canwrite() override;

    int putc(int c) override;

    // real_file_descriptor(int fd, int flags);
    real_file_descriptor(std::shared_ptr<mem_file> file, int fd, int flags);
};

class std_file_descriptor : public file_descriptor {
public:
    off_t lseek(off_t offset, int whence) override;

    inline char *gets(char *s, int size, int &eof_seen) override {
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

    inline size_t readobj(void *buf, size_t size, size_t nitem, int ungotchar) override {
        check_assert(false, fprintf(stderr, "readobj for std file not implemented"));
        return 0;
    }

    inline int getc() override {
        check_assert(false, fprintf(stderr, "getc for std file not implemented"));
        return EOF;
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

    inline int printf(const char *format, va_list ap) override {
        return EOF;
    }

    inline bool canread() override {
        return true;
    }

    inline bool canwrite() override {
        return true;
    }

    int putc(int c) override {
        return EOF;
    }

    inline explicit stdin_file_descriptor(int fd) : std_file_descriptor(fd, STDIN_FILE) {};
};

class stdout_file_descriptor : public std_file_descriptor {
public:
    ssize_t write(const void *buf, size_t count) override;

    int puts(const char *s) override;

    int printf(const char *format, va_list ap) override;

    inline bool canread() override {
        return false;
    }

    inline bool canwrite() override {
        return false;
    }

    inline int putc(int c) override {
        return c;
    }

    inline explicit stdout_file_descriptor(int fd) : std_file_descriptor(fd, STDOUT_FILE) {};
};


#endif //LLVM_ACCMUT_IO_FD_H
