//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_UNSUPPORTEDFILE_H
#define LLVM_UNSUPPORTEDFILE_H

class UnsupportedFile : public BaseFile {
    inline void dump(FILE *f) override {
        fprintf(f, "unsupported\n");
    }
};

#endif //LLVM_UNSUPPORTEDFILE_H
