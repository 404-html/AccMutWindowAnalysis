//
// Created by Sirui Lu on 2019-03-19.
//

#ifndef LLVM_INSTRUMENTMAIN_H
#define LLVM_INSTRUMENTMAIN_H

#include "llvm/Transforms/AccMut/Mutation.h"
#include "llvm/Transforms/AccMut/printIR.h"

#include "llvm/IR/Module.h"

#include <vector>
#include <unordered_map>
#include <string>

using namespace llvm;
using namespace std;

class InstrumentMainPass : public ModulePass {
    void instrument(Function &F);

public:
    static char ID;

    InstrumentMainPass();

    bool runOnModule(Module &M) override;
};

#endif //LLVM_INSTRUMENTMAIN_H
