//
// Created by Sirui Lu on 2019-03-19.
//

#ifndef LLVM_PRINTPASS_H
#define LLVM_PRINTPASS_H


#include "llvm/Transforms/AccMut/Mutation.h"
#include "llvm/Transforms/AccMut/printIR.h"

#include "llvm/IR/Module.h"

#include <vector>
#include <unordered_map>
#include <string>

using namespace llvm;
using namespace std;

class PrintPass : public ModulePass {
    const char *suffix;
public:
    static char ID;
    PrintPass();
    PrintPass(const char *suffix);
    bool runOnModule(Module &M) override;
};

#endif //LLVM_PRINTPASS_H
