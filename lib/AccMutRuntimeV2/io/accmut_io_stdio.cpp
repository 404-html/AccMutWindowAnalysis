//
// Created by lusirui on 19-3-10.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/accmut_io.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_stdio_support.h>

#define io_log_err(...) do {\
    fprintf(stderr, "Error [MUT: %d]: ", MUTATION_ID);\
    fprintf(stderr, __VA_ARGS__);\
} while (0)

static char buf_ori[1000000];

extern "C" {

ACCMUTV2_FILE *__accmutv2__fopen(const char *path, const char *mode) {
    int flags;
    switch (mode[0]) {
        case 'r':
            flags = 0;
            break;
        case 'w':
            flags = O_CREAT | O_TRUNC;
            break;
        case 'a':
            flags = O_CREAT | O_APPEND;
            break;
        default:
            io_log_err("Unknown fopen mode %s", "mode");
            exit(-1);
    }
    if (mode[1] == '+') {
        flags |= O_RDWR;
    } else if (mode[1] != 0) {
        io_log_err("Unknown fopen mode %s", "mode");
        exit(-1);
    } else if (mode[0] == 'r') {
        flags |= O_RDONLY;
    } else {
        flags |= O_WRONLY;
    }
    ACCMUTV2_FILE *file;
    if (MUTATION_ID == 0) {
        file = new ACCMUTV2_FILE();
        file->orifile = fopen(path, mode);
        if (!file->orifile) {
            delete file;
            return nullptr;
        }
        file->fd = fileno(file->orifile);

        __accmutv2__register(file->fd, flags);
    } else {
        file = new ACCMUTV2_FILE();
        file->orifile = nullptr;
        file->fd = __accmutv2__open(path, flags, 0666);
    }
    file->unget_char = EOF;
    file->eof_seen = false;
    return file;
}

int __accmutv2__fclose(ACCMUTV2_FILE *fp) {
    int ret = __accmutv2__deregister(fp->fd);
    if (MUTATION_ID == 0)
        ret &= fclose(fp->orifile);
    else check_null(fp->orifile);
    delete fp;
    return ret;
}

int __accmutv2__feof(ACCMUTV2_FILE *fp) {
    int ret = fp->eof_seen;
    int ori_ret = only_origin(::feof(fp->orifile));
    check_samebool(ret, ori_ret);
    return ret;
}

int __accmutv2__fileno(ACCMUTV2_FILE *fp) {
    return fp->fd;
}

char *__accmutv2__fgets(char *buf, int size, ACCMUTV2_FILE *fp) {
    char *ret = buf;
    if (size <= 0) {
        ret = nullptr;
    } else if (size == 1) {
        buf[0] = 0;
        ret = buf;
    } else {
        if (fp->eof_seen)
            ret = nullptr;
        if (fp->unget_char != EOF) {
            buf[0] = fp->unget_char;
            fp->unget_char = EOF;
            char *r1 = __accmutv2__fdgets__nosync(fp->fd, buf + 1, size - 1);
            if (!r1)
                fp->eof_seen = true;
        } else {
            ret = __accmutv2__fdgets__nosync(fp->fd, buf, size);
            if (!ret)
                fp->eof_seen = true;
        }
    }
    char *ori_ret = only_origin(::fgets(buf_ori, size, fp->orifile));
    if (!ret || !ori_ret) {
        check_null(ret);
        check_null(ori_ret);
        return nullptr;
    }
    check_streq(ret, ori_ret);
    return ret;
}

int __accmutv2__fputs(const char *buf, ACCMUTV2_FILE *fp) {
    int ret = __accmutv2__fdputs__nosync(fp->fd, buf);
    int ori_ret = only_origin(::fputs(buf, fp->orifile));
    if (ret == EOF) {
        check_eq(ori_ret, EOF);
    } else {
        check_neq(ori_ret, EOF);
    }
    return ret;
}

int __accmutv2__ungetc(int c, ACCMUTV2_FILE *fp) {
    int ret = fp->unget_char = c;
    fp->eof_seen = false;
    int ori_ret = only_origin(::ungetc(c, fp->orifile));
    check_eq(ret, ori_ret);
    return c;
}

int __accmutv2__fseek(ACCMUTV2_FILE *fp, long offset, int whence) {
    fp->unget_char = EOF;
    fp->eof_seen = false;
    off_t ret = __accmutv2__lseek__nosync(fp->fd, offset, whence);
    off_t ori_ret = only_origin(::fseek(fp->orifile, offset, whence));
    check_eq(ret, ori_ret);
    return (int) ret;
}

long __accmutv2__ftell(ACCMUTV2_FILE *fp) {
    return __accmutv2__fseek(fp, 0, SEEK_CUR);
}

void __accmutv2__rewind(ACCMUTV2_FILE *fp) {
    (void) __accmutv2__fseek(fp, 0L, SEEK_SET);
}

}
