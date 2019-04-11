add_llvm_library(LLVMAccMutRuntimeV3
        accmut_arith_common.cpp
        accmut_async_sig_safe_string.c
        accmut_config.c
        accmut_dma_fork.c
        accmut_GoodVar.cpp
        accmut_GoodVar_fork.c
        io/accmut_io_fd.cpp
        io/accmut_io_stdio.cpp
        io/accmut_io_fdmap.cpp
        io/accmut_io_unix.cpp
        io/accmut_io_stdio_support.cpp
        io/accmut_io_buf_ori.cpp
        io/accmut_io_scanf.cpp
        io/accmut_io_init.cpp
        io/accmut_io_static_typechecking.cpp
        fs/accmut_fs_simulate.cpp
        fs/accmut_fs_syscall.cpp
        accmut_init_early.cpp

        checking/check.cpp

        filesystem/inodemapping/caching.cpp
        filesystem/inodemapping/linking.cpp
        filesystem/inodemapping/cwd.cpp
        filesystem/datastructure/DirectoryFile.cpp
        filesystem/datastructure/inode.cpp
        filesystem/rawapi.cpp
        filesystem/utils/dump.cpp

        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/AccMutRuntimeV3
        )
