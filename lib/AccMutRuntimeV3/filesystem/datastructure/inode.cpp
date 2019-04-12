//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/datastructure/inode.h>

void inode::dump(FILE *f) {
    fprintf(f, "ACCESS: %o\n", meta.st_mode & (~S_IFMT));
    fprintf(f, "INODE: %lld\n", meta.st_ino);
    if (content)
        content->dump(f);
    else
        fprintf(f, "Unknown content\n");
}
