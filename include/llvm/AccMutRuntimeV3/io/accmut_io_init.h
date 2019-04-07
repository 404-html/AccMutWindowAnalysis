//
// Created by Sirui Lu on 2019-03-15.
//

#ifndef LLVM_ACCMUT_IO_INIT_H
#define LLVM_ACCMUT_IO_INIT_H

class _InitFDMap {
    static bool once;
public:
    _InitFDMap();
};

static _InitFDMap _init_fdmap;
#endif //LLVM_ACCMUT_IO_INIT_H
