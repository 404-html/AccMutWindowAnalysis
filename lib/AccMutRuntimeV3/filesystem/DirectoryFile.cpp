//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/DirectoryFile.h>

void DirectoryFile::dump(FILE *f) {
    for (auto d: dirents) {
        fprintf(f, "%ld\t%s\n", (long)d.d_ino, d.d_name);
    }
}
