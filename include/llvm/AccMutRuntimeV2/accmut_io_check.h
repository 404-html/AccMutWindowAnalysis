//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_CHECK_H
#define LLVM_ACCMUT_IO_CHECK_H

#include <stdio.h>
#include <string.h>

#define CHECKED
#ifdef CHECKED
#define check_assert(b, dumper) do {\
    if (MUTATION_ID == 0) {\
        if (!(b)) {\
            FILE *f = fopen("/tmp/accmut_check.log", "a");\
            fprintf(f, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            fprintf(stderr, "%s:%s:%d check failed\n", __FILE__, __FUNCTION__, __LINE__);\
            fprintf(stderr, "Dump:\n");\
            (void)dumper;\
            fclose(f);\
        }\
    }\
} while (0)
#else
#define check_assert(b, dumper)
#endif
#define only_origin(x) (MUTATION_ID == 0 ? x : 0)

void dump_mem(void *a, void *b, size_t size) {
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

#define check_mem(a, b, size) check_assert(!memcmp(a, b, size), dump_mem(a, b, size))
#define check_eq(a, b) check_assert((a) == (b), \
    fprintf(stderr, "\tLeft = %ld, Right = %ld\n", (long)a, (long)b)\
)
#define check_neq(a, b) check_assert((a) != (b), \
    fprintf(stderr, "\tLeft = %ld, Right = %ld\n", (long)a, (long)b)\
)
#define check_null(a) check_assert(!(a),\
    fprintf(stderr, "Pointer is not null\n")\
)
#define check_nonnull(a) check_assert((a),\
    fprintf(stderr, "Pointer is null\n")\
)

#endif //LLVM_ACCMUT_IO_CHECK_H
