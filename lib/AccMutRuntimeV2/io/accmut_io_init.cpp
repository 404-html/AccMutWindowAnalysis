//
// Created by Sirui Lu on 2019-03-15.
//

#include "llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h"
#include "llvm/AccMutRuntimeV2/io/accmut_io_stdio.h"
#include "llvm/AccMutRuntimeV2/io/accmut_io_init.h"


bool _InitFDMap::once = false;

void init_stdfile(ACCMUTV2_FILE **fp, int fd, const char *mode) {
    *fp = new ACCMUTV2_FILE();
    (*fp)->fd = fd;
    if (isatty(fd)) {
        switch (fd) {
            case 0:
                (*fp)->orifile = stdin;
                break;
            case 1:
                (*fp)->orifile = stdout;
                break;
            case 2:
                (*fp)->orifile = stderr;
        }
    } else {
        (*fp)->orifile = fdopen(fd, mode);
    }
    (*fp)->unget_char = EOF;
    (*fp)->eof_seen = false;
}

_InitFDMap::_InitFDMap() {
    if (!once) {
        memset(fdmap, 0, sizeof(file_descriptor *) * 65536);
        if (isatty(0))
            fdmap[0] = new stdin_file_descriptor(0);
        else
            fdmap[0] = new real_file_descriptor(0, O_RDONLY);
        if (isatty(1))
            fdmap[1] = new stdout_file_descriptor(1);
        else
            fdmap[1] = new real_file_descriptor(1, O_WRONLY);
        if (isatty(2))
            fdmap[2] = new stdout_file_descriptor(2);
        else
            fdmap[2] = new real_file_descriptor(2, O_WRONLY);
        opened_file_list = {0, 1, 2};
        init_stdfile(&accmutv2_stdin, 0, "r");
        init_stdfile(&accmutv2_stdout, 1, "w");
        init_stdfile(&accmutv2_stderr, 2, "w");
        once = true;
    }
}
