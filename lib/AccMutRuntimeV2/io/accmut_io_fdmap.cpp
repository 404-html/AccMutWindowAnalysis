//
// Created by lusirui on 19-3-11.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h>

fdmap_t &get_fdmap() {
    static file_descriptor *_fdmap[65536];
    return _fdmap;
}

std::list<int> &get_opened_file_list() {
    static std::list<int> _opened_file_list;
    return _opened_file_list;
}

