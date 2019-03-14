//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_FDMAP_H
#define LLVM_ACCMUT_IO_FDMAP_H

#include "accmut_io_fd.h"
#include <list>

typedef file_descriptor *fdmap_t[65536];

fdmap_t &get_fdmap();

std::list<int> &get_opened_file_list();

#define fdmap get_fdmap()
#define opened_file_list get_opened_file_list()

#endif //LLVM_ACCMUT_IO_FDMAP_H
