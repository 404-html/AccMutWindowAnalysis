//
// Created by Sirui Lu on 2019-03-15.
//

#ifndef LLVM_RENAMEPASS_H
#define LLVM_RENAMEPASS_H

#include "llvm/IR/Module.h"
#include <map>

using namespace llvm;

class RenamePass : public ModulePass {
public:
    static char ID;

    bool runOnModule(Module &M) override;

    RenamePass();

private:
    void renameType();

    Type *rename(Type *ty);

    void renameGlobals();

    void rewriteFunctions();

    Module *theModule;
    StructType *accmut_file_ty;
    StructType *file_ty;
    std::map<StructType *, StructType *> tymap;
    std::map<Function *, Function *> funcmap;
    std::map<GlobalVariable *, GlobalVariable *> gvmap;
    std::vector<std::string> toSkip;

    void initSkip();

    void initFunc();
};

#endif //LLVM_RENAMEPASS_H
