//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_CHECK_H
#define LLVM_ACCMUT_IO_CHECK_H

#include "../accmut_config.h"
#include "accmut_io_restrict.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

#define only_origin(x) (MUTATION_ID == 0 ? x : 0)
#define only_origin_do(x) if (MUTATION_ID == 0) {x;}


#ifdef __cplusplus
extern "C" {
#endif

FILE *__check_fopen(const char *restrict path, const char *restrict mode);
int __check_fclose(FILE *stream);
int __check_fprintf(FILE *restrict stream, const char *restrict format, ...);
extern FILE *__check_stderr;

#ifdef __cplusplus
};
#endif

#ifndef NOCHECK
#define check_assert(b, dumper) only_origin_do(\
    do {\
        if (!(b)) {\
            FILE *f = __check_fopen("/tmp/accmut_check.log", "a");\
            __check_fprintf(f, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            __check_fprintf(__check_stderr, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            __check_fprintf(__check_stderr, "Dump:\n");\
            (void)dumper;\
            __check_fclose(f);\
        }\
    } while(0))
#else
#define check_assert(b, dumper)
#endif

inline void dump_mem(void *a, void *b, size_t size) {
    auto *a1 = (char *) a;
    auto *b1 = (char *) b;
    for (size_t i = 0; i < size; ++i) {
        if (a1[i] != b1[i]) {
            __check_fprintf(__check_stderr, "\tMemory difference starts form pos %ld\n", size);
            __check_fprintf(__check_stderr, "\tLeft: ");
            for (size_t j = i; j < std::min(size, i + 128); ++j) {
                __check_fprintf(__check_stderr, "%02x ", a1[j]);
            }
            __check_fprintf(__check_stderr, "\n\tRight: ");
            for (size_t j = i; j < std::min(size, i + 128); ++j) {
                __check_fprintf(__check_stderr, "%02x ", b1[j]);
            }
            __check_fprintf(__check_stderr, "\n");
            break;
        }
    }
}

extern char __buf[2][1000];

template<typename T>
std::string toString(const T &t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

#define check_mem(a, b, size) check_assert(!memcmp(a, b, size), dump_mem(a, b, size))
#define check_eq(a, b) check_assert((a) == (b), \
    __check_fprintf(__check_stderr, "\t%s = %s, %s = %s\n", #a, toString(a).c_str(), #b, toString(b).c_str())\
)
#define check_neq(a, b) check_assert((a) != (b), \
    __check_fprintf(__check_stderr, "\t%s = %s = %s\n", #a, #b, toString(a).c_str())\
)
#define check_null(a) check_assert(!(a),\
    __check_fprintf(__check_stderr, "Pointer %s is not null\n", #a)\
)
#define check_nonnull(a) check_assert((a),\
    __check_fprintf(__check_stderr, "Pointer %s is null\n", #a)\
)
#define check_true(a) check_assert(!(a),\
    __check_fprintf(__check_stderr, "%s should be true\n", #a)\
)
#define check_false(a) check_assert((a),\
    __check_fprintf(__check_stderr, "%s should be false\n", #a)\
)
#define check_streq(a, b) check_assert(strcmp(a, b) == 0, \
    __check_fprintf(__check_stderr, "\t%s\tis %s\n\t%s\tis %s\n", #a, a, #b, b)\
)
#define check_samebool(a, b) check_assert(!(a) == !(b),\
    __check_fprintf(__check_stderr, "%s(%d) and %s(%d) should be same in bool\n", #a, !!a, #b, !!b)\
)

#endif //LLVM_ACCMUT_IO_CHECK_H
