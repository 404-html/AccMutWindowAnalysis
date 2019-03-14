set(LLVM_LINK_COMPONENTS AccMutRuntimeV2)

add_llvm_tool(test-runtime
        main.cpp

        DEPENDS
        intrinsics_gen
        LLVMAccMutRuntimeV2
        )