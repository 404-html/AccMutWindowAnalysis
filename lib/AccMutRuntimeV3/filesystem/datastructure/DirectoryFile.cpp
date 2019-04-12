//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/datastructure/DirectoryFile.h>
#include <dirent.h>

void DirectoryFile::dump(FILE *f) {
    for (auto d: dirents) {
        fprintf(f, "%ld\t%s", (long) d.d_ino, d.d_name);
        if (d.d_type == DT_LNK)
            fprintf(f, "->\n");
        else if (d.d_type == DT_DIR)
            fprintf(f, "/\n");
        else if (d.d_type == DT_REG)
            fprintf(f, "\n");
        else
            fprintf(f, "?\n");
    }
}
