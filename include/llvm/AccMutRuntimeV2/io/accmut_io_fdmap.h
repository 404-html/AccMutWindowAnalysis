//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_FDMAP_H
#define LLVM_ACCMUT_IO_FDMAP_H

#include "accmut_io_fd.h"
#include <list>

extern file_descriptor *fdmap[65536];
extern std::list<int> opened_file_list;

#endif //LLVM_ACCMUT_IO_FDMAP_H
