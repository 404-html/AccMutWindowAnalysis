#include <list>
#include <stdio.h>
#include <algorithm>

extern "C" {
#include "llvm/AccMutRuntime/accmut_io.h"
}

static std::list<ACCMUT_FILE *> opened_file_lst;

extern "C" {
void __accmut__io__close__ori() {
    for (auto fp : opened_file_lst) {
        if (fp->orifile) {
            fclose(fp->orifile);
            fp->orifile = NULL;
        }
    }
}

void __accmut__io__register(ACCMUT_FILE *ptr) {
    opened_file_lst.push_back(ptr);
}

void __accmut__io__deregister(ACCMUT_FILE *ptr) {
    auto it = std::find(opened_file_lst.begin(), opened_file_lst.end(), ptr);
    if (it == opened_file_lst.end())
        return;
    opened_file_lst.erase(it);
}
}
