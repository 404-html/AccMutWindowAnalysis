//
// Created by Sirui Lu on 2019-03-14.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_check.h>

FILE *__check_fopen(const char *restrict path, const char *restrict mode) {
    return fopen(path, mode);
}

int __check_fclose(FILE *stream) {
    return fclose(stream);
}

int __check_fprintf(FILE *restrict stream, const char *restrict format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

FILE *__check_stderr = stderr;
