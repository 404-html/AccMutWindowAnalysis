add_llvm_library(LLVMAccMutRuntimeV2
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
        checking/check.cpp
        io/accmut_io_init.cpp
        io/accmut_io_static_typechecking.cpp
        fs/accmut_fs_simulate.cpp
        fs/accmut_fs_syscall.cpp
        accmut_init_early.cpp

        filesystem/caching.cpp
        filesystem/DirectoryFile.cpp

        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/AccMutRuntimeV2
        )
