//
// Created by Sirui Lu on 2019-03-14.
//

#include "test-stdio.h"

int main() {
    // system("rm -rf /tmp/accmut_test");
    // mkdir("/tmp/accmut_test", 0755);
    if (!access("/tmp/accmut_test", F_OK))
        mkdir("tmp/accmut_test", 0755);
    chdir("/tmp/accmut_test");
    //unixrunner or1;
    // stdio_positioning_runner for1;
    // stdio_file_status_runner fsr1;
    // stdio_unix_interoperate_runner fuir1;
    // stdio_input_runner fir1;
    // stdio_scanf_runner ssr1;
    // stdio_fread_fwrite_runner sffr1;
    stdio_printf_runner spr1;
}