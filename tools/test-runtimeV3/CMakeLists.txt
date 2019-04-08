set(LLVM_LINK_COMPONENTS AccMutRuntimeV3)

add_llvm_tool(test-runtimeV3
        main.cpp

        DEPENDS
        intrinsics_gen
        LLVMAccMutRuntimeV3
        )