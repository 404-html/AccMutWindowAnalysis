//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_BASEFILE_H
#define LLVM_BASEFILE_H

#include <stdio.h>

class BaseFile {
public:
    virtual ~BaseFile() = default;
    virtual void dump(FILE *f) = 0;
};

#endif //LLVM_BASEFILE_H
