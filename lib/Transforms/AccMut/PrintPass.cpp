//
// Created by Sirui Lu on 2019-03-19.
//

#include <llvm/Transforms/AccMut/PrintPass.h>

PrintPass::PrintPass(const char *suffix): ModulePass(ID), suffix(suffix) {};
PrintPass::PrintPass() : PrintPass(".ll") {}
char PrintPass::ID = 0;

bool PrintPass::runOnModule(Module &M) {
    printIR(M, suffix);
    return false;
}
static RegisterPass<PrintPass> X("AccMut-PrintPass", "AccMut - Print IR");
