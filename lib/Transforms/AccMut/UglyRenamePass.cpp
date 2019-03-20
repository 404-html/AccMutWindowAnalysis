//
// Created by Sirui Lu on 2019-03-20.
//

#include "llvm/Transforms/AccMut/UglyRenamePass.h"
#include "llvm/Transforms/AccMut/printIR.h"
#include <set>
#include <map>
#include <string>
#include <vector>

UglyRenamePass::UglyRenamePass() : ModulePass(ID) {
}

bool UglyRenamePass::runOnModule(Module &M) {
    printIR(M, ".ori.ll");

    std::set<std::string> accmut_catched_func{
            "fclose",
            "feof",
            "fileno",
            "fflush",
            "ferror",
            "fseek",
            "ftell",
            "fseeko",
            "ftello",
            "rewind",
            "fread",
            "fgets",
            "fgetc",
            "getc",
            "ungetc",
            "vfscanf",
            "fscanf",
            "scanf",
            "fputc",
            "vfprintf",
            "fprintf",
            "printf",
            "perror",
            "lseek",
            "fopen",
            "fputs",
            "freopen",
            "fputs",
            "fwrite",
            "open",
            "creat",
            "close",
            "read",
            "write"
    };
#ifdef __APPLE__
    std::map<std::string, std::string> gvmp = {
            {"__stdinp",  "accmutv2_stdin"},
            {"__stdoutp", "accmutv2_stdout"},
            {"__stderrp", "accmutv2_stderr"}
    };
    std::set<std::string> accmut_catched_func_mac_alias;
    for (auto &s : accmut_catched_func) {
        accmut_catched_func_mac_alias.insert("\01_" + s);
        llvm::errs() << "\01_" + s << "\n";
    }
#else
#endif

    for (Function &F : M) {
        if (accmut_catched_func.find(F.getName().str()) != accmut_catched_func.end())
            F.setName("__accmutv2__" + F.getName());
#ifdef __APPLE__
        else if (accmut_catched_func_mac_alias.find(F.getName().str()) != accmut_catched_func_mac_alias.end())
            F.setName("__accmutv2__" + F.getName().substr(2));
#endif
    }
    for (GlobalVariable &gv : M.globals()) {
        if (gvmp.find(gv.getName()) != gvmp.end())
            gv.setName(gvmp[gv.getName()]);
    }
    printIR(M, ".new.ll");
    return true;
}


char UglyRenamePass::ID = 0;
static RegisterPass<UglyRenamePass> X("AccMut-UglyRenamePass", "AccMut - Rename FILE");
