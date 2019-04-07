//
// Created by Sirui Lu on 2019-03-15.
//

#include "llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h"
#include "llvm/AccMutRuntimeV2/io/accmut_io_stdio.h"
#include "llvm/AccMutRuntimeV2/io/accmut_io_init.h"
#include <fcntl.h>
#include <exception>


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

#ifdef __APPLE__

static int get_path(int fd, char *buf) {
    return fcntl(fd, F_GETPATH, buf);
}

#else
#endif

void set_std_descriptor(int fd) {
    if (isatty(fd)) {
        if (fd == 0)
            fdmap[fd] = new stdin_file_descriptor(fd);
        else
            fdmap[fd] = new stdout_file_descriptor(fd);
    } else {
        char buf[PATH_MAX];
        if (get_path(fd, buf) < 0) {
            exit(-2);
        }
        auto t = fs_query(buf, 0);
        if (!t) {
            exit(-3);
        }
        int flags = O_WRONLY;
        if (fd == 0)
            flags = O_RDONLY;
        fdmap[fd] = new real_file_descriptor(t, fd, flags);
    }
}

_InitFDMap::_InitFDMap() {
    if (!once) {
        memset(fdmap, 0, sizeof(file_descriptor *) * 65536);
        set_std_descriptor(0);
        set_std_descriptor(1);
        set_std_descriptor(2);
        opened_file_list = {0, 1, 2};
        init_stdfile(&accmutv2_stdin, 0, "r");
        init_stdfile(&accmutv2_stdout, 1, "w");
        init_stdfile(&accmutv2_stderr, 2, "w");
        once = true;
    }
}
