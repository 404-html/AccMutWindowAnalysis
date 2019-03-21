//
// Created by Sirui Lu on 2019-03-20.
//

#ifndef LLVM_ACCMUT_FAKE_FS_H
#define LLVM_ACCMUT_FAKE_FS_H

#include <string>
#include "accmut_fs_memfile.h"
#include "../io/accmut_io_fd.h"
#include <memory>
#include <vector>

const ino_t ACCMUT_DELETED_INO = ~((ino_t) 0);


int fs_ldelete(const char *str);

int fs_delete(const char *str);

int fs_rename(const char *ori, const char *after);

std::shared_ptr<mem_file> fs_query(const char *str, bool trunc = false);

std::shared_ptr<mem_file> fs_query(ino_t, bool trunc = false);

#endif //LLVM_ACCMUT_FAKE_FS_H
