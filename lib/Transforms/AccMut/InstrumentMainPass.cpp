//
// Created by Sirui Lu on 2019-03-19.
//

#include <llvm/Transforms/AccMut/InstrumentMainPass.h>

#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>

InstrumentMainPass::InstrumentMainPass() : ModulePass(ID) {}

char InstrumentMainPass::ID = 0;

bool InstrumentMainPass::runOnModule(Module &M) {
    bool ret = false;
    for (Function &F : M) {
        if (F.getName() == "main") {
            ret = true;

            int idx = 0;
            Argument *argc = nullptr;
            Argument *argv = nullptr;
            for (auto &arg : F.args()) {
                if (idx == 0)
                    argc = &arg;
                if (idx == 1)
                    argv = &arg;
                ++idx;
            }
            if (!argv) {
                llvm::errs() << "Main should have argc & argv\n";
                exit(-1);
            }
            IntegerType *i32ty = IntegerType::get(M.getContext(), 32);
            IntegerType *i8ty = IntegerType::get(M.getContext(), 8);
            PointerType *i8pty = PointerType::get(i8ty, 0);
            Function *atoifunc = M.getFunction("atoi");
            if (!atoifunc) {
                std::vector<Type *> atoiargs;
                atoiargs.push_back(i8pty);
                FunctionType *atoity = FunctionType::get(
                        IntegerType::get(M.getContext(), 32),
                        atoiargs,
                        false
                );
                if (atoity->isVarArg()) {
                    exit(-1);
                }
                atoifunc = Function::Create(
                        atoity,
                        GlobalValue::ExternalLinkage,
                        "atoi",
                        &M
                );
                atoifunc->setCallingConv(CallingConv::C);
                AttributeList funccallattr;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(M.getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    funccallattr = AttributeList::get(M.getContext(), Attrs);
                }
                atoifunc->setAttributes(funccallattr);
                atoifunc->setOnlyReadsMemory();
                atoifunc->setDoesNotThrow();
                atoifunc->arg_begin()->addAttr(Attribute::AttrKind::NoCapture);
            }


            GlobalVariable *testid = M.getGlobalVariable("TEST_ID");
            if (!testid) {
                testid = new GlobalVariable(
                        M,
                        i32ty,
                        false,
                        GlobalVariable::LinkageTypes::ExternalLinkage,
                        nullptr,
                        "TEST_ID",
                        nullptr
                );
            }

            Function *initfunc = M.getFunction("__accmut__init");
            if (!initfunc) {
                FunctionType *initty = FunctionType::get(
                        Type::getVoidTy(M.getContext()),
                        {},
                        false
                );
                initfunc = Function::Create(
                        initty,
                        GlobalValue::ExternalLinkage,
                        "__accmut__init",
                        &M
                );
                initfunc->setCallingConv(CallingConv::C);

                AttributeList initattr;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(M.getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    initattr = AttributeList::get(M.getContext(), Attrs);
                }
                initfunc->setAttributes(initattr);
            }


            BasicBlock *front = &F.front();
            BasicBlock *bb = BasicBlock::Create(M.getContext(), "", &F, front);
            auto *i3 = BinaryOperator::CreateNSWAdd(/*argc*/ UndefValue::get(i32ty),
                                                             ConstantInt::get(i32ty, ~0U, true),
                                                             "",
                                                             bb);

            argc->replaceAllUsesWith(i3);
            i3->setOperand(0, argc);

            auto *i4 = new SExtInst(i3, IntegerType::get(M.getContext(), 64), "", bb);

            auto *i5 = GetElementPtrInst::Create(i8pty, argv, {i4}, "", bb);
            i5->setIsInBounds(true);

            auto *i6 = new LoadInst(i5, "", false, 8, bb);

            auto *i7 = CallInst::Create(atoifunc, {i6}, "", bb);

            i7->setTailCallKind(CallInst::TailCallKind::TCK_Tail);

            auto *st1 = new StoreInst(i7, testid, false, 4, bb);

            auto *callinit = CallInst::Create(initfunc, std::vector<Value *>{}, "", bb);

            callinit->setTailCallKind(CallInst::TailCallKind::TCK_Tail);

            auto *st2 = new StoreInst(ConstantPointerNull::get(i8pty), i5, false, 8, bb);

            auto *br = BranchInst::Create(front, bb);



            /*
            st2->insertBefore(first);
            callinit->insertBefore(st2);
            st1->insertBefore(callinit);
            i7->insertBefore(st1);
            i6->insertBefore(i7);
            i5->insertBefore(i6);
            i4->insertBefore(i5);
            i3->insertBefore(i4);
             */
            llvm::errs() << F << "\n";
            if (verifyModule(M, &(llvm::errs()))) {
                llvm::errs() << "FATAL!!!! failed to verify\n";
                exit(-1);
            }
            return true;
        }
    }
    return false;
}

void InstrumentMainPass::instrument(Function &F) {

}

static RegisterPass<InstrumentMainPass> X("AccMut-InstrumentMainPass", "AccMut - InstrumentMain");