//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_SYMBOLICLINKFILE_H
#define LLVM_SYMBOLICLINKFILE_H

class SymbolicLinkFile : public BaseFile {
    std::string target;
public:
    inline SymbolicLinkFile(std::string target) : target(target) {}
    inline void dump(FILE *f) override {
        fprintf(f, "linkto\t%s\n", target.c_str());
    }
};

#endif //LLVM_SYMBOLICLINKFILE_H
