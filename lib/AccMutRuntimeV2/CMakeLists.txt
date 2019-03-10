add_llvm_library(AccMutRuntimeV2
        accmut_arith_common.cpp
        accmut_async_sig_safe_string.c
        accmut_config.c
        accmut_dma_fork.c
        accmut_GoodVar.cpp
        accmut_GoodVar_fork.c
        accmut_io.c
        accmut_io_fd.cpp
        accmut_io_cleanup.cpp

        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/AccMutRuntimeV2
        )
