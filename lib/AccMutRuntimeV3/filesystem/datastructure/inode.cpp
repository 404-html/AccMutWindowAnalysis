//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/datastructure/inode.h>

void inode::dump(FILE *f) {
    if (content)
        content->dump(f);
    else
        fprintf(stderr, "Unknown content\n");
}
