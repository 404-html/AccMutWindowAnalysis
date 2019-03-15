//
// Created by Sirui Lu on 2019-03-15.
//

#include <llvm/Transforms/AccMut/RenamePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/TypeFinder.h>
#include <stack>
#include <llvm/IR/Operator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>

#ifdef __APPLE__
#define FILE_STRUCT "struct.__sFILE"
#endif

RenamePass::RenamePass() : ModulePass(ID) {

}

bool RenamePass::runOnModule(Module &M) {
    theModule = &M;
    accmut_file_ty = theModule->getTypeByName("struct.ACCMUTV2_FILE");
    file_ty = theModule->getTypeByName(FILE_STRUCT);
    if (!file_ty) {
        llvm::errs() << "NO DEFINITION FOR FILE FOUND\n";
        return false;
    }
    if (!accmut_file_ty) {
        accmut_file_ty = StructType::create(
                theModule->getContext(),
                {IntegerType::get(theModule->getContext(), 32),
                 IntegerType::get(theModule->getContext(), 32),
                 file_ty,
                 IntegerType::get(theModule->getContext(), 32),
                 IntegerType::get(theModule->getContext(), 32),
                },
                "struct.ACCMUTV2_FILE");
    }
    renameType();
    renameGlobals();
    initFunc();
    return true;
}

Type *RenamePass::rename(Type *ty) {
    if (isa<StructType>(ty)) {
        auto it = tymap.find(dyn_cast<StructType>(ty));
        if (it == tymap.end())
            return ty;
        return it->second;
    } else if (isa<IntegerType>(ty)) {
        return ty;
    } else if (isa<VectorType>(ty)) {
        return ty;
    } else if (isa<PointerType>(ty)) {
        auto *pt = dyn_cast<PointerType>(ty);
        return PointerType::getUnqual(rename(pt->getElementType()));
    } else if (isa<FunctionType>(ty)) {
        auto *ft = dyn_cast<FunctionType>(ty);
        Type *resultTy = rename(ft->getReturnType());
        std::vector<Type *> params;
        for (Type *t : ft->params()) {
            params.push_back(rename(t));
        }
        return FunctionType::get(resultTy, params, ft->isVarArg());
    } else {
        return ty;
    }
}

void RenamePass::initSkip() {
#ifdef __APPLE__
    toSkip = {"struct.__sFILE",
              "struct.__sFILEX",
              "struct.__sbuf"};
#else
    toSkip = {"struct.__sFILE",
              "struct.__sFILEX",
              "struct.__sbuf"};
#endif
    toSkip.push_back("struct.Mutation");
    toSkip.push_back("struct.RegMutInfo");
}

void RenamePass::renameType() {
    llvm::TypeFinder StructTypes, ST1;
    StructTypes.run(*theModule, true);
    initSkip();

    tymap[file_ty] = accmut_file_ty;
    for (auto *sty : StructTypes) {
        if (std::find(toSkip.begin(), toSkip.end(), sty->getName()) != toSkip.end())
            continue;
        if (sty->isOpaque())
            continue;
        tymap[sty] = StructType::create(theModule->getContext(), sty->getName().str() + ".new");
    }
    llvm::errs() << "\n";
    for (auto *sty : StructTypes) {
        if (std::find(toSkip.begin(), toSkip.end(), sty->getName()) != toSkip.end())
            continue;
        if (sty->isOpaque())
            continue;
        std::vector<Type *> nsubty;
        llvm::errs() << *sty << "\n";
        for (auto *t : sty->elements()) {
            nsubty.push_back(rename(t));
        }
        auto *t = theModule->getTypeByName(sty->getName().str() + ".new");
        t->setBody(nsubty);
    }

    ST1.run(*theModule, true);
    for (auto *sty : ST1) {
        auto *t = theModule->getTypeByName(sty->getName().str() + ".new");
        if (t)
            llvm::errs() << *t << "\n";
    }
}

void RenamePass::initFunc() {
    for (Function &F : *theModule) {
        funcmap[&F] = nullptr;
    }
    for (auto &F : funcmap) {
        Function *newfunc = Function::Create(
                dyn_cast<FunctionType>(rename(F.first->getFunctionType())),
                F.first->getLinkage(),
                F.first->getName() + "__renamed",
                theModule);
        funcmap[F.first] = newfunc;
        newfunc->setAttributes(F.first->getAttributes());
        newfunc->setCallingConv(F.first->getCallingConv());
    }
}

void RenamePass::renameGlobals() {
    for (GlobalValue &gv : theModule->globals()) {
        if (!isa<GlobalVariable>(&gv)) {
            if (isa<Function>(&gv))
                continue;
            llvm::errs() << "Unknown gv type: " << gv << "\n";
            exit(-1);
        }
        gvmap[dyn_cast<GlobalVariable>(&gv)] = nullptr;
    }
    for (auto &gv : gvmap) {
        auto origv = gv.first;
        auto newgv = new GlobalVariable(*theModule, rename(origv->getType()),
                                        origv->isConstant(),
                                        origv->getLinkage(), nullptr,
                                        origv->getName() + "__renamed",
                                        origv, origv->getThreadLocalMode());
        gvmap[origv] = newgv;
    }
}

Function *RenamePass::rewriteFunctions() {
    for (auto &p : funcmap) {
        Function *orifn = p.first;
        Function *newfn = p.second;
        std::map<Value *, Value *> valmap;
        for (auto b1 = orifn->arg_begin(), b2 = newfn->arg_begin(); b1 != orifn->arg_end(); ++b1, ++b2) {
            valmap[&*b1] = &*b2;
        }
        std::stack<Instruction *> inststack;
        for (BasicBlock &bb : *orifn) {
            valmap[&bb] = BasicBlock::Create(theModule->getContext());
            for (Instruction &inst : bb) {
                inststack.push(&inst);
                valmap[&inst] = nullptr;
            }
        }

        std::function<Value *(Value *)>
                rewriteValue,
                rewriteBinaryOperator,
                rewriteBranchInst,
                rewriteCallInst,
                rewriteCmpInst,
                rewriteGetElementPtrInst,
                rewritePHINode,
                rewriteStoreInst,
                rewriteAllocaInst,
                rewriteCastInst,
                rewriteLoadInst,
                rewriteUnreachableInst;


        rewriteValue = [&](Value *arg) -> Value * {
            if (isa<BinaryOperator>(arg)) {
                return rewriteBinaryOperator(arg);
            } else if (isa<BranchInst>(arg)) {
                return rewriteBranchInst(arg);
            } else if (isa<CallInst>(arg)) {
                return rewriteCallInst(arg);
            } else if (isa<CmpInst>(arg)) {
                return rewriteCmpInst(arg);
            } else if (isa<GetElementPtrInst>(arg)) {
                return rewriteGetElementPtrInst(arg);
            } else if (isa<PHINode>(arg)) {
                return rewritePHINode(arg);
            } else if (isa<StoreInst>(arg)) {
                return rewriteStoreInst(arg);
            } else if (isa<AllocaInst>(arg)) {
                return rewriteAllocaInst(arg);
            } else if (isa<CastInst>(arg)) {
                return rewriteCastInst(arg);
            } else if (isa<LoadInst>(arg)) {
                return rewriteLoadInst(arg);
            } else if (isa<UnreachableInst>(arg)) {
                return rewriteUnreachableInst(arg);
            } else {
                llvm::errs() << "Unknown instr " << *arg << "\n";
                exit(-1);
            }
        };

        rewriteBinaryOperator = [&](Value *arg) -> Value * {
            BinaryOperator *bop = dyn_cast<BinaryOperator>(arg);
            BinaryOperator *newbop = BinaryOperator::Create(bop->getOpcode(),
                                                            rewriteValue(bop->getOperand(0)),
                                                            rewriteValue(bop->getOperand(1)));
            return newbop;
        };

        rewriteBranchInst = [&](Value *arg) -> Value * {
            auto *binst = dyn_cast<BranchInst>(arg);
            if (binst->isUnconditional()) {
                return BranchInst::Create(dyn_cast<BasicBlock>(valmap[binst->getSuccessor(0)]));
            } else {
                return BranchInst::Create(
                        dyn_cast<BasicBlock>(valmap[binst->getSuccessor(0)]),
                        dyn_cast<BasicBlock>(valmap[binst->getSuccessor(1)]),
                        binst->getCondition());
            }
        };

        while (!inststack.empty()) {
            Instruction *inst = inststack.top();
            inststack.pop();
            if (valmap[inst])
                continue;

        }
    }
}

char RenamePass::ID = 0;
static RegisterPass<RenamePass> X("AccMut-instrument", "AccMut - Rename FILE");