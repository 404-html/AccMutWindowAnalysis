//
// Created by Sirui Lu on 2019-03-21.
//

#ifndef LLVM_ACCMUT_FS_MEMFILE_H
#define LLVM_ACCMUT_FS_MEMFILE_H

#include <vector>

struct mem_file {
    size_t size;
    std::vector<char> data;

    inline mem_file() {
        data.resize(10);
        size = 0;
    }
    // mode_t mode;
};

#endif //LLVM_ACCMUT_FS_MEMFILE_H
