//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_CWD_H
#define LLVM_CWD_H

#include <llvm/AccMutRuntimeV3/checking/panic.h>
#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/caching.h>
#include <llvm/AccMutRuntimeV3/filesystem/utils/path.h>
#include <sys/param.h>
#include <unistd.h>
#include <string>
#include <deque>

void chdir_internal(const char *path);

ino_t getwdino_internal();

int getcwd_internal(char *buf, size_t size);

void getrelawd_internal(char *buf);

int getwd_internal(char *buf);

#endif //LLVM_CWD_H
