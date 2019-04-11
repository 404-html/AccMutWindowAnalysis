//
// Created by Sirui Lu on 2019-04-11.
//

#include <llvm/AccMutRuntimeV3/filesystem/utils/dump.h>

static std::set <ino_t> dumped;

void dump_helper(FILE *f, ino_t ino) {
    fprintf(f, "Start dump: %lu\n", (unsigned long) ino);
    dumped.insert(ino);
    auto ptr = inomap[ino];
    ptr->dump(f);
    if (S_ISDIR(ptr->meta.st_mode)) {
        for (const auto &t : *std::static_pointer_cast<DirectoryFile>(ptr->content)) {
            if (t.d_name[0] == '.')
                continue;
            if (dumped.find(t.d_ino) == dumped.end())
                if (inomap.find(t.d_ino) != inomap.end())
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