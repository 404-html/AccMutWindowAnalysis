//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_CHECK_H
#define LLVM_ACCMUT_IO_CHECK_H

#include "../accmut_config.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

#define only_origin(x) (MUTATION_ID == 0 ? x : 0)
#define only_origin_do(x) if (MUTATION_ID == 0) {x;}
#define CHECKED
#ifdef CHECKED
#define check_assert(b, dumper) only_origin_do(\
    do {\
        if (!(b)) {\
            FILE *f = fopen("/tmp/accmut_check.log", "a");\
            fprintf(f, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            fprintf(stderr, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            fprintf(stderr, "Dump:\n");\
            (void)dumper;\
            fclose(f);\
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
            fprintf(stderr, "\tMemory difference starts form pos %ld\n", size);
            fprintf(stderr, "\tLeft: ");
            for (size_t j = i; j < std::min(size, i + 128); ++j) {
                fprintf(stderr, "%02x ", a1[j]);
            }
            fprintf(stderr, "\n\tRight: ");
            for (size_t j = i; j < std::min(size, i + 128); ++j) {
                fprintf(stderr, "%02x ", b1[j]);
            }
            fprintf(stderr, "\n");
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
    fprintf(stderr, "\t%s = %s, %s = %s\n", #a, #b, toString(a).c_str(), toString(b).c_str())\
)
#define check_neq(a, b) check_assert((a) != (b), \
    fprintf(stderr, "\t%s = %s = %s\n", #a, #b, toString(a).c_str())\
)
#define check_null(a) check_assert(!(a),\
    fprintf(stderr, "Pointer %s is not null\n", #a)\
)
#define check_nonnull(a) check_assert((a),\
    fprintf(stderr, "Pointer %s is null\n", #a)\
)
#define check_true(a) check_assert(!(a),\
    fprintf(stderr, "%s should be true\n", #a)\
)
#define check_false(a) check_assert((a),\
    fprintf(stderr, "%s should be false\n", #a)\
)
#define check_streq(a, b) check_assert(strcmp(a, b) == 0, \
    fprintf(stderr, "\t%s\tis %s\n\t%s\tis %s\n", #a, a, #b, b)\
)
#define check_samebool(a, b) check_assert(!(a) == !(b),\
    fprintf(stderr, "%s(%d) and %s(%d) should be same in bool\n", #a, !!a, #b, !!b)\
)

#endif //LLVM_ACCMUT_IO_CHECK_H
