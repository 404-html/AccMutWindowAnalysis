//
// Created by lusirui on 19-3-11.
//

#include <llvm/AccMutRuntimeV2/io/accmut_io_fd.h>
#include <llvm/AccMutRuntimeV2/io/accmut_io_fdmap.h>

file_descriptor *fdmap[65536];
std::list<int> opened_file_list;

class _InitFDMap {
    static bool once;
public:
    _InitFDMap();
};

bool _InitFDMap::once = false;
_InitFDMap _init_fdmap;

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
        once = true;
    }
}