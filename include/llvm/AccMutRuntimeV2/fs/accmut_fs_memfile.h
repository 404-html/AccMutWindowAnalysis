//
// Created by Sirui Lu on 2019-03-21.
//

#ifndef LLVM_ACCMUT_FS_MEMFILE_H
#define LLVM_ACCMUT_FS_MEMFILE_H

#include <vector>

struct mem_file {
    size_t size;
    std::vector<char> data;
};

#endif //LLVM_ACCMUT_FS_MEMFILE_H
