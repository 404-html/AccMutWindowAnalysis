//
// Created by Sirui Lu on 2019-04-11.
//

#include <llvm/AccMutRuntimeV3/filesystem/utils/dump.h>
#include <set>
#include <memory>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/DirectoryFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/caching.h>

static std::set <ino_t> dumped;

void dump_helper(FILE *f, ino_t ino) {
    fprintf(f, "Start dump: %lu\n", (unsigned long) ino);
    dumped.insert(ino);
    auto ptr = ino2inode(ino);
    ptr->dump(f);
    if (S_ISDIR(ptr->meta.st_mode)) {
        for (const auto &t : *std::static_pointer_cast<DirectoryFile>(ptr->content)) {
            if (t.d_name[0] == '.')
                continue;
            if (dumped.find(t.d_ino) == dumped.end())
                if (ino2inode(t.d_ino))
                    dump_helper(f, t.d_ino);
        }
    }
    fprintf(f, "End dump: %lu\n", (unsigned long) ino);
}

void dump_cache(FILE *f, const char *base) {
    dumped.clear();
    ino_t ino_base = cache_tree(base);
    dump_helper(f, ino_base);
}