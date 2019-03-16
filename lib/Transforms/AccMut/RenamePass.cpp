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
#include <llvm/IR/IRBuilder.h>

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
    rewriteFunctions();
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
    toSkip.emplace_back("struct.Mutation");
    toSkip.emplace_back("struct.RegMutInfo");
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
    llvm::errs() << "\n--- RENAMING TYPES ---";
    for (auto *sty : StructTypes) {
        if (std::find(toSkip.begin(), toSkip.end(), sty->getName()) != toSkip.end())
            continue;
        if (sty->isOpaque())
            continue;
        std::vector<Type *> nsubty;
        for (auto *t : sty->elements()) {
            nsubty.push_back(rename(t));
        }
        auto *t = theModule->getTypeByName(sty->getName().str() + ".new");
        t->setBody(nsubty);
        llvm::errs() << "RENAMING\t" << *sty << "\tTO\t" << *t << "\n";
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
        auto *type = dyn_cast<PointerType>(origv->getType());
        if (!type) {
            llvm::errs() << "No pointer type: " << origv->getType() << "\n";
            exit(-1);
        }
        auto *basetype = type->getElementType();
        auto newgv = new GlobalVariable(*theModule, rename(basetype),
                                        origv->isConstant(),
                                        origv->getLinkage(), nullptr,
                                        origv->getName() + "__renamed",
                                        origv, origv->getThreadLocalMode());
        gvmap[origv] = newgv;
    }
}

void RenamePass::rewriteFunctions() {
    for (auto &p : funcmap) {
        Function *orifn = p.first;
        Function *newfn = p.second;
        if (p.first->isDeclaration())
            continue;
        std::map<Value *, Value *> valmap;
        for (auto b1 = orifn->arg_begin(), b2 = newfn->arg_begin(); b1 != orifn->arg_end(); ++b1, ++b2) {
            valmap[&*b1] = &*b2;
        }
        for (BasicBlock &bb : *orifn) {
            valmap[&bb] = BasicBlock::Create(theModule->getContext(), "");
            for (Instruction &inst : bb) {
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
                rewriteReturnInst,
                rewriteStoreInst,
                rewriteAllocaInst,
                rewriteCastInst,
                rewriteLoadInst,
                rewriteUnreachableInst,
                rewriteConstantExpr,
                rewriteGlobalObject;

        rewriteValue = [&](Value *arg) -> Value * {
            if (!arg)
                return nullptr;
            auto iter = valmap.find(arg);
            if (iter != valmap.end()) {
                if (iter->second) {
                    llvm::errs() << "FOUND\t" << *arg << "\tTO\t" << *(iter->second) << "\n";
                    return iter->second;
                }
            }
            llvm::errs() << "REWRTING\t" << *arg << "\n";
            Value *ret;
            if (isa<BinaryOperator>(arg)) {
                ret = rewriteBinaryOperator(arg);
            } else if (isa<BranchInst>(arg)) {
                ret = rewriteBranchInst(arg);
            } else if (isa<CallInst>(arg)) {
                ret = rewriteCallInst(arg);
            } else if (isa<CmpInst>(arg)) {
                ret = rewriteCmpInst(arg);
            } else if (isa<GetElementPtrInst>(arg)) {
                ret = rewriteGetElementPtrInst(arg);
            } else if (isa<PHINode>(arg)) {
                ret = rewritePHINode(arg);
            } else if (isa<StoreInst>(arg)) {
                ret = rewriteStoreInst(arg);
            } else if (isa<ReturnInst>(arg)) {
                ret = rewriteReturnInst(arg);
            } else if (isa<AllocaInst>(arg)) {
                ret = rewriteAllocaInst(arg);
            } else if (isa<CastInst>(arg)) {
                ret = rewriteCastInst(arg);
            } else if (isa<LoadInst>(arg)) {
                ret = rewriteLoadInst(arg);
            } else if (isa<UnreachableInst>(arg)) {
                ret = rewriteUnreachableInst(arg);
            } else if (isa<ConstantData>(arg)) {
                ret = arg;
            } else if (isa<ConstantExpr>(arg)) {
                ret = rewriteConstantExpr(arg);
            } else if (isa<GlobalObject>(arg)) {
                ret = rewriteGlobalObject(arg);
            } else {
                if (isa<Operator>(arg))
                    llvm::errs() << "Is a operator\n";
                llvm::errs() << dyn_cast<Operator>(arg)->getOpcode() << "\n";
                if (dyn_cast<Instruction>(arg))
                    llvm::errs() << "Can cast\n";
                if (isa<ConstantExpr>(arg))
                    llvm::errs() << "Can cast constant\n";
                llvm::errs() << "Unknown instr " << *arg << "\n";
                exit(-1);
            }
            llvm::errs() << "REWROTE\t" << *arg << "\tTO\t" << *ret << "\n";
            if (iter != valmap.end()) {
                valmap[arg] = ret;
            }
            return ret;
        };

        rewriteBinaryOperator = [&](Value *arg) -> Value * {
            auto *bop = dyn_cast<BinaryOperator>(arg);
            auto *newbop = BinaryOperator::Create(bop->getOpcode(),
                                                  rewriteValue(bop->getOperand(0)),
                                                  rewriteValue(bop->getOperand(1)));
            if (bop->getOpcode() < 11 || bop->getOpcode() > 28) {
                llvm::errs() << "Unknown opcode for binary operator " << bop->getOpcode() << "\n";
                exit(-1);
            }
            newbop->copyIRFlags(bop);
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
                        rewriteValue(binst->getCondition()));
            }
        };

        rewriteCallInst = [&](Value *arg) -> Value * {
            auto *cinst = dyn_cast<CallInst>(arg);
            auto *calledFunc = cinst->getCalledFunction();
            if (calledFunc) {
                std::vector<Value *> args;
                for (auto &v : cinst->arg_operands()) {
                    args.push_back(rewriteValue(v));
                }
                auto *newinst = CallInst::Create(funcmap[calledFunc], args);
                newinst->setAttributes(cinst->getAttributes());
                newinst->setTailCallKind(cinst->getTailCallKind());
                return newinst;
            } else {
                llvm::errs() << "Indirect call:\t" << cinst << "\n";
                exit(-1);
            }
        };

        rewriteCmpInst = [&](Value *arg) -> Value * {
            auto *cinst = dyn_cast<CmpInst>(arg);
            auto *newinst = CmpInst::Create(
                    cinst->getOpcode(),
                    cinst->getPredicate(),
                    rewriteValue(cinst->getOperand(0)),
                    rewriteValue(cinst->getOperand(1)));
            return newinst;
        };

        rewriteGetElementPtrInst = [&](Value *arg) -> Value * {
            auto *gepinst = dyn_cast<GetElementPtrInst>(arg);
            std::vector<Value *> idxList;
            for (auto &v : gepinst->indices()) {
                idxList.push_back(rewriteValue(v));
            }
            auto *ptop = rewriteValue(gepinst->getPointerOperand());
            auto *resultTy = rename(gepinst->getSourceElementType());
            auto *newinst = GetElementPtrInst::Create(
                    resultTy,
                    ptop,
                    idxList
            );
            newinst->setIsInBounds(gepinst->isInBounds());
            return newinst;
        };

        rewritePHINode = [&](Value *arg) -> Value * {
            auto *phinode = dyn_cast<PHINode>(arg);
            std::vector<BasicBlock *> bbs;
            std::vector<Value *> values;
            for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
                bbs.push_back(dyn_cast<BasicBlock>(valmap[phinode->getIncomingBlock(i)]));
                values.push_back(rewriteValue(phinode->getIncomingValue(i)));
            }
            auto *newinst = PHINode::Create(
                    rename(phinode->getType()),
                    phinode->getNumIncomingValues()
            );
            for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
                newinst->setIncomingBlock(i, bbs[i]);
                newinst->setIncomingValue(i, values[i]);
            }
            return newinst;
        };

        rewriteReturnInst = [&](Value *arg) -> Value * {
            auto *retinst = dyn_cast<ReturnInst>(arg);
            auto *newinst = ReturnInst::Create(
                    theModule->getContext(),
                    rewriteValue(retinst->getReturnValue()));

            return newinst;
        };

        rewriteStoreInst = [&](Value *arg) -> Value * {
            auto *sinst = dyn_cast<StoreInst>(arg);
            auto *newinst = new StoreInst(
                    rewriteValue(sinst->getValueOperand()),
                    rewriteValue(sinst->getPointerOperand()),
                    sinst->isVolatile(),
                    sinst->getAlignment(),
                    sinst->getOrdering(),
                    sinst->getSyncScopeID()
            );
            return newinst;
        };

        rewriteAllocaInst = [&](Value *arg) -> Value * {
            auto ainst = dyn_cast<AllocaInst>(arg);
            auto *newinst = new AllocaInst(
                    rename(ainst->getType()->getElementType()),
                    0,
                    rewriteValue(ainst->getArraySize()),
                    ainst->getAlignment());
            return newinst;
        };

        rewriteCastInst = [&](Value *arg) -> Value * {
            auto *cinst = dyn_cast<CastInst>(arg);
            auto *newinst = CastInst::Create(
                    cinst->getOpcode(),
                    rewriteValue(cinst->getOperand(0)),
                    rename(cinst->getDestTy()));
            return newinst;
        };

        rewriteLoadInst = [&](Value *arg) -> Value * {
            auto linst = dyn_cast<LoadInst>(arg);
            auto *newinst = new LoadInst(
                    rename(linst->getType()),
                    rewriteValue(linst->getPointerOperand()),
                    "",
                    linst->isVolatile(),
                    linst->getAlignment(),
                    linst->getOrdering(),
                    linst->getSyncScopeID()
            );
            return newinst;
        };

        rewriteUnreachableInst = [&](Value *arg) -> Value * {
            return arg;
        };

        rewriteConstantExpr = [&](Value *arg) -> Value * {
            auto *cexpr = dyn_cast<ConstantExpr>(arg);
            std::vector<Value *> oprewrite;

            for (unsigned i = 0; i < cexpr->getNumOperands(); ++i) {
                oprewrite.push_back(rewriteValue(cexpr->getOperand(i)));
            }

            auto *ty = rename(cexpr->getType());
            if (cexpr->isCast()) {
                if (oprewrite.size() != 1) {
                    llvm::errs() << "Cast has more than one op: " << *cexpr << "\n";
                    exit(-1);
                }
                auto *op = dyn_cast<Constant>(oprewrite[0]);
                if (!op) {
                    llvm::errs() << "Not a constant: " << *oprewrite[0] << "\n";
                }
                return ConstantExpr::getCast(cexpr->getOpcode(), op, ty);
            }

            switch (cexpr->getOpcode()) {
                case Instruction::GetElementPtr: {
                    auto *inst = dyn_cast<GetElementPtrInst>(cexpr->getAsInstruction());
                    auto *pty = rename(inst->getSourceElementType());
                    auto *constant = dyn_cast<Constant>(rewriteValue(inst->getPointerOperand()));
                    if (!constant) {
                        llvm::errs() << "Not a constant for gep constant op: " << *constant << "\n";
                        exit(-1);
                    }
                    std::vector<Constant *> idxList;
                    for (auto &idx : inst->indices()) {
                        auto *t = dyn_cast<Constant>(rewriteValue(idx));
                        if (!t) {
                            llvm::errs() << "Not a constant for idx: " << *t << "\n";
                            exit(-1);
                        }
                        idxList.push_back(t);
                    }
                    bool isInBounds = inst->isInBounds();
                    return ConstantExpr::getGetElementPtr(
                            pty,
                            constant,
                            idxList,
                            isInBounds
                    );
                }
                default:
                    llvm::errs() << "Unknown opcode " << cexpr->getOpcode() << "\n";
                    exit(-1);
            }
        };

        rewriteGlobalObject = [&](Value *arg) -> Value * {
            if (isa<Function>(arg)) {
                return funcmap[cast<Function>(arg)];
            } else {
                return gvmap[cast<GlobalVariable>(arg)];
            }
        };


        for (BasicBlock &bb : *orifn) {
            valmap[&bb] = BasicBlock::Create(theModule->getContext());
            for (Instruction &inst : bb) {
                llvm::errs() << "GO INTO\t" << inst << "\n";
                rewriteValue(&inst);
            }
        }

        // rebuild
        for (BasicBlock &bb : *orifn) {
            auto *newbb = dyn_cast<BasicBlock>(valmap[&bb]);
            // builder.SetInsertPoint(newbb);
            for (auto &inst : bb) {
                auto *newinst = dyn_cast<Instruction>(valmap[&inst]);
                // builder.Insert(newinst);
                newbb->getInstList().push_back(newinst);
            }
            newbb->insertInto(newfn);
        }

        /* relink br */
        for (BasicBlock &bb : *orifn) {
            auto *newbb = dyn_cast<BasicBlock>(valmap[&bb]);
            for (auto &inst : bb) {
                auto *br = dyn_cast<BranchInst>(&inst);
                if (br) {
                    auto *newbr = dyn_cast<BranchInst>(valmap[br]);
                    if (!newbr) {
                        llvm::errs() << "Should be br: " << *newbr << "\n";
                        exit(-1);
                    }
                    for (unsigned i = 0; i < br->getNumSuccessors(); ++i) {
                        newbr->setSuccessor(i, dyn_cast<BasicBlock>(valmap[br->getSuccessor(i)]));
                    }
                }
            }
        }
    }
}

char RenamePass::ID = 0;
static RegisterPass<RenamePass> X("AccMut-instrument", "AccMut - Rename FILE");