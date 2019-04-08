//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/inode.h>

void inode::dump(FILE *f) {
    content->dump(f);
}
