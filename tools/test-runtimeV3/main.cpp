//
// Created by Sirui Lu on 2019-03-14.
//

#include <llvm/AccMutRuntimeV3/filesystem/caching.h>
#include <stdio.h>

int main() {
    // system("rm -rf /tmp/accmut_test");
    // mkdir("/tmp/accmut_test", 0755);
    /*if (!access("/tmp/accmut_test", F_OK))
        mkdir("tmp/accmut_test", 0755);
    chdir("/tmp/accmut_test");*/

    dump_cache(stderr);
    auto t = (long) fs_cache("/tmp/test/b/a");
    perror("a");
    fprintf(stdout, "%ld\n", t);
    dump_cache(stderr);


}