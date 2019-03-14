//
// Created by Sirui Lu on 2019-03-14.
//

#include "test-stdio.h"
#include "test-unix.h"

int main() {
    system("rm -rf /tmp/accmut_test");
    mkdir("/tmp/accmut_test", 0755);
    chdir("/tmp/accmut_test");
    unixrunner or1;
    stdiorunner for1;
}