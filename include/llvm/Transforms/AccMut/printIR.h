//
// Created by Sirui Lu on 2019-03-19.
//

#ifndef LLVM_PRINTIR_H
#define LLVM_PRINTIR_H

#include <llvm/Support/raw_ostream.h>
#include "llvm/IR/Module.h"
using namespace llvm;
using namespace std;

inline void printIR(Module &M, const char *suffix) {
    std::string ir;
    raw_string_ostream os(ir);
    os << M;
    os.flush();
    FILE *f = fopen((std::string(M.getName()) + suffix).c_str(), "w");
    fputs(ir.c_str(), f);
    fclose(f);
}

#endif //LLVM_PRINTIR_H
