//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_REGULARFILE_H
#define LLVM_REGULARFILE_H

#include "BaseFile.h"
#include <vector>

class RegularFile: public BaseFile {
    std::vector<char> data;
    size_t len;
public:
    inline RegularFile(std::vector<char> data, size_t len)
        : data(std::move(data)), len(len) {}
    inline void dump(FILE *f) override {
        fprintf(f, "size: %lu\n", len);
    }
};

#endif //LLVM_REGULARFILE_H
