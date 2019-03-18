//
// Created by Sirui Lu on 2019-03-15.
//

#include <llvm/Transforms/AccMut/RenamePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/TypeFinder.h>
#include <stack>
#include <set>
#include <llvm/IR/Operator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>

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
    rewriteGlobalInitalizers();
    renameBack();
    if (verifyModule(M, &(llvm::errs()))) {
        llvm::errs() << "FATAL!!!!!! failed to verify\n";
        exit(-1);
    }
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
        auto *at = dyn_cast<VectorType>(ty);
        return VectorType::get(rename(at->getElementType()), at->getNumElements());
    } else if (isa<ArrayType>(ty)) {
        auto *at = dyn_cast<ArrayType>(ty);
        return ArrayType::get(rename(at->getElementType()), at->getNumElements());
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
        tymap[sty] = StructType::create(theModule->getContext(), "__renamed__" + sty->getName().str());
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
        auto *t = theModule->getTypeByName("__renamed__" + sty->getName().str());
        t->setBody(nsubty);
        llvm::errs() << "RENAMING\t" << *sty << "\tTO\t" << *t << "\n";
    }
}

void RenamePass::initFunc() {
    std::vector<Function *> vec;
    for (Function &F : *theModule) {
        vec.push_back(&F);
    }
    for (auto &F : vec) {
        Function *newfunc = Function::Create(
                dyn_cast<FunctionType>(rename(F->getFunctionType())),
                F->getLinkage(),
                "__renamed__" + F->getName(),
                theModule);
        funcmap[F] = newfunc;
        newfunc->setAttributes(F->getAttributes());
        newfunc->setCallingConv(F->getCallingConv());
        newfunc->copyAttributesFrom(F);
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
#ifdef __APPLE__
        std::map<StringRef, StringRef> stdfiles{
                {"__stdinp",  "accmutv2_stdin"},
                {"__stdoutp", "accmutv2_stdout"},
                {"__stderrp", "accmutv2_stderr"}
        };
#else
#endif
        GlobalVariable *newgv;
        auto iter = stdfiles.find(origv->getName());
        if (iter != stdfiles.end()) {
            newgv = new GlobalVariable(*theModule, rename(basetype),
                                       origv->isConstant(),
                                       origv->getLinkage(), nullptr,
                                       iter->second,
                                       origv, origv->getThreadLocalMode());
        } else {
            newgv = new GlobalVariable(*theModule, rename(basetype),
                                       origv->isConstant(),
                                       origv->getLinkage(), nullptr,
                                       "__renamed__" + origv->getName(),
                                       origv, origv->getThreadLocalMode());
        }
        newgv->setAttributes(origv->getAttributes());
        gvmap[origv] = newgv;
        llvm::errs() << "RENAMING GLOBAL\t" << *origv << "\tTO\t" << *newgv << "\n";
    }
}

Value *RenamePass::rewriteBinaryOperator(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "BinaryOperator\n";
    auto *bop = dyn_cast<BinaryOperator>(arg);
    auto *newbop = BinaryOperator::Create(bop->getOpcode(),
                                          rewriteValue(bop->getOperand(0), valmap),
                                          rewriteValue(bop->getOperand(1), valmap));
    if (bop->getOpcode() < 11 || bop->getOpcode() > 28) {
        llvm::errs() << "Unknown opcode for binary operator " << bop->getOpcode() << "\n";
        exit(-1);
    }
    newbop->copyIRFlags(bop);
    return newbop;
};

Value *RenamePass::rewriteBranchInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "BranchInst\n";
    auto *binst = dyn_cast<BranchInst>(arg);
    if (binst->isUnconditional()) {
        return BranchInst::Create(dyn_cast<BasicBlock>(valmap[binst->getSuccessor(0)]));
    } else {
        return BranchInst::Create(
                dyn_cast<BasicBlock>(valmap[binst->getSuccessor(0)]),
                dyn_cast<BasicBlock>(valmap[binst->getSuccessor(1)]),
                rewriteValue(binst->getCondition(), valmap));
    }
};

Value *RenamePass::rewriteCallInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "CallInst\n";
    auto *cinst = dyn_cast<CallInst>(arg);
    auto *calledFunc = cinst->getCalledFunction();

    std::vector<Value *> args;
    for (auto &v : cinst->arg_operands()) {
        args.push_back(rewriteValue(v, valmap));
    }
    CallInst *newinst;
    if (calledFunc) {
        newinst = CallInst::Create(funcmap[calledFunc], args);
    } else {
        newinst = CallInst::Create(rewriteValue(cinst->getCalledValue(), valmap), args);
    }
    newinst->setAttributes(cinst->getAttributes());
    newinst->setTailCallKind(cinst->getTailCallKind());
    return newinst;
};

Value *RenamePass::rewriteCmpInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "CmpInst\n";
    auto *cinst = dyn_cast<CmpInst>(arg);
    auto *newinst = CmpInst::Create(
            cinst->getOpcode(),
            cinst->getPredicate(),
            rewriteValue(cinst->getOperand(0), valmap),
            rewriteValue(cinst->getOperand(1), valmap));
    return newinst;
};

Value *RenamePass::rewriteGetElementPtrInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "GetElementPtrInst\n";
    auto *gepinst = dyn_cast<GetElementPtrInst>(arg);
    std::vector<Value *> idxList;
    for (auto &v : gepinst->indices()) {
        idxList.push_back(rewriteValue(v, valmap));
    }
    auto *ptop = rewriteValue(gepinst->getPointerOperand(), valmap);
    auto *resultTy = rename(gepinst->getSourceElementType());
    auto *newinst = GetElementPtrInst::Create(
            resultTy,
            ptop,
            idxList
    );
    newinst->setIsInBounds(gepinst->isInBounds());
    return newinst;
};

Value *RenamePass::rewritePHINode(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "PHINode\n";
    auto *phinode = dyn_cast<PHINode>(arg);
    /*
    std::vector<BasicBlock *> bbs;
    std::vector<Value *> values;
    for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
        bbs.push_back(dyn_cast<BasicBlock>(valmap[phinode->getIncomingBlock(i)]));
        values.push_back(rewriteValue(phinode->getIncomingValue(i), valmap));
    }*/
    auto *newinst = PHINode::Create(
            rename(phinode->getType()),
            phinode->getNumIncomingValues()
    );
    /*
    for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
        newinst->addIncoming(values[i], bbs[i]);
    }*/
    return newinst;
};

Value *RenamePass::rewriteReturnInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "ReturnInst\n";
    auto *retinst = dyn_cast<ReturnInst>(arg);
    auto *newinst = ReturnInst::Create(
            theModule->getContext(),
            rewriteValue(retinst->getReturnValue(), valmap));

    return newinst;
};

Value *RenamePass::rewriteStoreInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "StoreInst\n";
    auto *sinst = dyn_cast<StoreInst>(arg);
    auto *newinst = new StoreInst(
            rewriteValue(sinst->getValueOperand(), valmap),
            rewriteValue(sinst->getPointerOperand(), valmap),
            sinst->isVolatile(),
            sinst->getAlignment(),
            sinst->getOrdering(),
            sinst->getSyncScopeID()
    );
    return newinst;
};

Value *RenamePass::rewriteAllocaInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "AllocaInst\n";
    auto ainst = dyn_cast<AllocaInst>(arg);
    auto *newinst = new AllocaInst(
            rename(ainst->getType()->getElementType()),
            0,
            rewriteValue(ainst->getArraySize(), valmap),
            ainst->getAlignment());
    return newinst;
};

Value *RenamePass::rewriteCastInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "CastInst\n";
    auto *cinst = dyn_cast<CastInst>(arg);
    auto *newinst = CastInst::Create(
            cinst->getOpcode(),
            rewriteValue(cinst->getOperand(0), valmap),
            rename(cinst->getDestTy()));
    return newinst;
};

Value *RenamePass::rewriteLoadInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "LoadInst\n";
    auto linst = dyn_cast<LoadInst>(arg);
    auto *newinst = new LoadInst(
            rename(linst->getType()),
            rewriteValue(linst->getPointerOperand(), valmap),
            "",
            linst->isVolatile(),
            linst->getAlignment(),
            linst->getOrdering(),
            linst->getSyncScopeID()
    );
    return newinst;
};

Value *RenamePass::rewriteConstantExpr(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "ConstantExpr\n";
    auto *cexpr = dyn_cast<ConstantExpr>(arg);
    std::vector<Value *> oprewrite;

    for (unsigned i = 0; i < cexpr->getNumOperands(); ++i) {
        oprewrite.push_back(rewriteValue(cexpr->getOperand(i), valmap));
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
            auto *constant = dyn_cast<Constant>(rewriteValue(inst->getPointerOperand(), valmap));
            if (!constant) {
                llvm::errs() << "Not a constant for gep constant op: " << *constant << "\n";
                exit(-1);
            }
            std::vector<Constant *> idxList;
            for (auto &idx : inst->indices()) {
                auto *t = dyn_cast<Constant>(rewriteValue(idx, valmap));
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

Value *RenamePass::rewriteGlobalObject(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "GlobalObject\n";
    if (isa<Function>(arg)) {
        return funcmap[cast<Function>(arg)];
    } else if (isa<GlobalVariable>(arg)) {
        llvm::errs() << *arg << "\n";
        return gvmap[cast<GlobalVariable>(arg)];
    } else {
        llvm::errs() << "FUCK" << "\n";
        exit(-1);
    }
};

Value *RenamePass::rewriteConstantData(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "ConstantData\n";
    llvm::errs() << *arg << "\n";
    if (isa<ConstantPointerNull>(arg)) {
        auto *cpn = cast<ConstantPointerNull>(arg);
        auto *ty = dyn_cast<PointerType>(rename(cpn->getType()));
        return ConstantPointerNull::get(ty);
    } else if (isa<ConstantAggregateZero>(arg)) {
        return ConstantAggregateZero::get(rename(arg->getType()));
    } else if (isa<ConstantDataArray>(arg)) {
        auto *cda = cast<ConstantDataArray>(arg);
        auto *elety = cda->getElementType();
        if (elety->isFloatTy()) {
            std::vector<float> vec;
            for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                vec.push_back(cda->getElementAsFloat(i));
            }
            return ConstantDataArray::get(theModule->getContext(), vec);
        } else if (elety->isDoubleTy()) {
            std::vector<double> vec;
            for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                vec.push_back(cda->getElementAsDouble(i));
            }
            return ConstantDataArray::get(theModule->getContext(), vec);
        } else {
            auto *intty = cast<IntegerType>(elety);
            switch (intty->getBitWidth()) {
                case 8: {
                    std::vector<uint8_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint8_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataArray::get(theModule->getContext(), vec);
                }
                case 16: {
                    std::vector<uint16_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint16_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataArray::get(theModule->getContext(), vec);
                }
                case 32: {
                    std::vector<uint32_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint32_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataArray::get(theModule->getContext(), vec);
                }
                case 64: {
                    std::vector<uint64_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint64_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataArray::get(theModule->getContext(), vec);
                }
                default:
                    llvm::errs() << "Unsupported bit width: " << intty->getBitWidth() << "\n";
                    exit(-1);
            }
        }
    } else if (isa<ConstantDataVector>(arg)) {
        auto *cda = cast<ConstantDataVector>(arg);
        auto *elety = cda->getElementType();
        if (elety->isFloatTy()) {
            std::vector<float> vec;
            for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                vec.push_back(cda->getElementAsFloat(i));
            }
            return ConstantDataVector::get(theModule->getContext(), vec);
        } else if (elety->isDoubleTy()) {
            std::vector<double> vec;
            for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                vec.push_back(cda->getElementAsDouble(i));
            }
            return ConstantDataVector::get(theModule->getContext(), vec);
        } else {
            auto *intty = cast<IntegerType>(elety);
            switch (intty->getBitWidth()) {
                case 8: {
                    std::vector<uint8_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint8_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataVector::get(theModule->getContext(), vec);
                }
                case 16: {
                    std::vector<uint16_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint16_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataVector::get(theModule->getContext(), vec);
                }
                case 32: {
                    std::vector<uint32_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint32_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataVector::get(theModule->getContext(), vec);
                }
                case 64: {
                    std::vector<uint64_t> vec;
                    for (unsigned i = 0; i < cda->getNumElements(); ++i) {
                        vec.push_back((uint64_t) cda->getElementAsInteger(i));
                    }
                    return ConstantDataVector::get(theModule->getContext(), vec);
                }
                default:
                    llvm::errs() << "Unsupported bit width: " << intty->getBitWidth() << "\n";
                    exit(-1);
            }
        }
    } else if (isa<UndefValue>(arg)) {
        return UndefValue::get(rename(arg->getType()));
    } else {
        return arg;
    }
};

Value *RenamePass::rewriteSwitchInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "Switch\n";
    auto *sinst = dyn_cast<SwitchInst>(arg);
    auto *newinst = SwitchInst::Create(
            rewriteValue(sinst->getCondition(), valmap),
            cast<BasicBlock>(valmap[sinst->case_default()->getCaseSuccessor()]),
            sinst->getNumCases());
    for (auto t = sinst->case_begin();
         t != sinst->case_end();
         ++t) {
        newinst->addCase(t->getCaseValue(), cast<BasicBlock>(valmap[t->getCaseSuccessor()]));
    }
    return newinst;
}

Value *RenamePass::rewriteSelectInst(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "Select\n";
    auto *sinst = dyn_cast<SelectInst>(arg);
    auto *newinst = SelectInst::Create(
            rewriteValue(sinst->getCondition(), valmap),
            rewriteValue(sinst->getTrueValue(), valmap),
            rewriteValue(sinst->getFalseValue(), valmap)
    );
    return newinst;
}

Value *RenamePass::rewriteConstantAggregate(Value *arg, std::map<Value *, Value *> &valmap) {
    std::vector<Constant *> retvec;
    for (auto &t : dyn_cast<User>(arg)->operands()) {
        retvec.push_back(dyn_cast<Constant>(rewriteValue(dyn_cast<Value>(&t), valmap)));
    }
    if (isa<ConstantVector>(arg)) {
        return ConstantVector::get(retvec);
    } else if (isa<ConstantArray>(arg)) {
        return ConstantArray::get(dyn_cast<ArrayType>(rename(dyn_cast<ConstantArray>(arg)->getType())), retvec);
    } else {
        return ConstantStruct::get(dyn_cast<StructType>(rename(dyn_cast<ConstantStruct>(arg)->getType())), retvec);
    }
}

Value *RenamePass::rewriteValue(Value *arg, std::map<Value *, Value *> &valmap) {
    llvm::errs() << "Value\n";
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
        ret = rewriteBinaryOperator(arg, valmap);
    } else if (isa<BranchInst>(arg)) {
        ret = rewriteBranchInst(arg, valmap);
    } else if (isa<CallInst>(arg)) {
        ret = rewriteCallInst(arg, valmap);
    } else if (isa<CmpInst>(arg)) {
        ret = rewriteCmpInst(arg, valmap);
    } else if (isa<GetElementPtrInst>(arg)) {
        ret = rewriteGetElementPtrInst(arg, valmap);
    } else if (isa<PHINode>(arg)) {
        ret = rewritePHINode(arg, valmap);
    } else if (isa<StoreInst>(arg)) {
        ret = rewriteStoreInst(arg, valmap);
    } else if (isa<SwitchInst>(arg)) {
        ret = rewriteSwitchInst(arg, valmap);
    } else if (isa<SelectInst>(arg)) {
        ret = rewriteSelectInst(arg, valmap);
    } else if (isa<ReturnInst>(arg)) {
        ret = rewriteReturnInst(arg, valmap);
    } else if (isa<AllocaInst>(arg)) {
        ret = rewriteAllocaInst(arg, valmap);
    } else if (isa<CastInst>(arg)) {
        ret = rewriteCastInst(arg, valmap);
    } else if (isa<LoadInst>(arg)) {
        ret = rewriteLoadInst(arg, valmap);
    } else if (isa<UnreachableInst>(arg)) {
        ret = new UnreachableInst(theModule->getContext());
    } else if (isa<ConstantData>(arg)) {
        ret = rewriteConstantData(arg, valmap);
    } else if (isa<ConstantExpr>(arg)) {
        ret = rewriteConstantExpr(arg, valmap);
    } else if (isa<GlobalObject>(arg)) {
        ret = rewriteGlobalObject(arg, valmap);
    } else if (isa<ConstantAggregate>(arg)) {
        ret = rewriteConstantAggregate(arg, valmap);
    } else {
        llvm::errs() << "Unknown\n";
        if (isa<Operator>(arg)) {
            llvm::errs() << "Is a operator\n";
            llvm::errs() << dyn_cast<Operator>(arg)->getOpcode() << "\n";
        }
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

        for (BasicBlock &bb : *orifn) {
            valmap[&bb] = BasicBlock::Create(theModule->getContext());
            for (Instruction &inst : bb) {
                llvm::errs() << "GO INTO\t" << inst << "\n";
                rewriteValue(&inst, valmap);
            }
        }

        llvm::errs() << "Before rebuild\n";
        // rebuild
        for (BasicBlock &bb : *orifn) {
            auto *newbb = dyn_cast<BasicBlock>(valmap[&bb]);
            // builder.SetInsertPoint(newbb);
            for (auto &inst : bb) {
                auto *newinst = dyn_cast<Instruction>(valmap[&inst]);
                // builder.Insert(newinst);
                llvm::errs() << *valmap[&inst] << "\n";
                newbb->getInstList().push_back(newinst);
            }
            newbb->insertInto(newfn);
        }
        llvm::errs() << "After rebuild\n";

        /* relink BasicBlocks */
        // Don't modify, it's magic....
        // very important to break the dependency loop
        for (BasicBlock &bb : *orifn) {
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

                auto *phinode = dyn_cast<PHINode>(&inst);
                if (phinode) {
                    std::vector<BasicBlock *> bbs;
                    std::vector<Value *> values;
                    for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
                        bbs.push_back(dyn_cast<BasicBlock>(valmap[phinode->getIncomingBlock(i)]));
                        llvm::errs() << dyn_cast<BasicBlock>(valmap[phinode->getIncomingBlock(i)]) << "\n";
                        llvm::errs() << *(dyn_cast<BasicBlock>(valmap[phinode->getIncomingBlock(i)])) << "\n";
                        values.push_back(rewriteValue(phinode->getIncomingValue(i), valmap));
                    }
                    auto newinst = dyn_cast<PHINode>(valmap[phinode]);
                    for (unsigned i = 0; i < phinode->getNumIncomingValues(); ++i) {
                        newinst->addIncoming(values[i], bbs[i]);
                    }
                }

                auto *switchinst = dyn_cast<SwitchInst>(&inst);
                if (switchinst) {
                    auto newinst = dyn_cast<SwitchInst>(valmap[switchinst]);
                    newinst->setDefaultDest(dyn_cast<BasicBlock>(valmap[switchinst->getDefaultDest()]));
                    for (auto t = switchinst->case_begin(), n = newinst->case_begin();
                         t != switchinst->case_end() && n != newinst->case_end();
                         ++t, ++n) {
                        llvm::errs() << *(t->getCaseSuccessor()) << "\n";
                        llvm::errs() << *(valmap[t->getCaseSuccessor()]) << "\n";
                        n->setSuccessor(cast<BasicBlock>(valmap[t->getCaseSuccessor()]));
                        n->setValue(t->getCaseValue());
                    }
                }
            }
        }
        if (verifyFunction(*newfn, &(llvm::errs()))) {
            llvm::errs() << "FATAL!!!!!! failed to verify\n";
            llvm::errs() << *orifn << "\n";
            llvm::errs() << *newfn << "\n";
            exit(-1);
        }
    }
}

void RenamePass::rewriteGlobalInitalizers() {
    std::map<Value *, Value *> dummy;
    for (auto &gv : gvmap) {
        auto *origv = gv.first;
        auto *newgv = gv.second;
        newgv->copyAttributesFrom(origv);
        if (origv->hasInitializer()) {
            if (newgv->getName().startswith("accmutv2")) {
                llvm::errs() << "stdfiles should not have an initializer\n";
                exit(-1);
            }
            llvm::errs() << "REWRITE INIT FOR\t" << *newgv << "\n";
            llvm::errs() << "ORIGINAL\t" << *(origv->getInitializer()) << "\n";
            llvm::errs() << "NEW\t" << *rewriteValue(origv->getInitializer(), dummy) << "\n";
            newgv->setInitializer(dyn_cast<Constant>(rewriteValue(origv->getInitializer(), dummy)));
        } else {
            llvm::errs() << "NO INIT: " << *origv << "\n";
            llvm::errs() << "NO INIT(new): " << *newgv << "\n";
        }
    }
}

void RenamePass::renameBack() {
    for (auto &gv : gvmap) {
        gv.first->replaceAllUsesWith(UndefValue::get(gv.first->getType()));
        gv.first->removeFromParent();
        if (gv.second->getName().startswith("__renamed__"))
            gv.second->setName(gv.first->getName());
        gv.first->setInitializer(nullptr);
        gv.first->dropAllReferences();
    }
    std::set<std::string> accmut_catched_func{
            "fclose",
            "feof",
            "fileno",
            "fflush",
            "ferror",
            "fseek",
            "ftell",
            "fseeko",
            "ftello",
            "rewind",
            "fread",
            "fgets",
            "fgetc",
            "getc",
            "ungetc",
            "vfscanf",
            "fscanf",
            "scanf",
            "fputc",
            "vfprintf",
            "fprintf",
            "printf",
            "perror",
            "lseek",
            "fopen",
            "fputs",
            "freopen",
            "fputs",
            "fwrite",
            "open",
            "creat",
            "close",
            "read",
            "write"
    };


#ifdef __APPLE__
    std::set<std::string> accmut_catched_func_mac_alias;
    for (auto &s : accmut_catched_func) {
        accmut_catched_func_mac_alias.insert("\01_" + s);
        llvm::errs() << "\01_" + s << "\n";
    }
#endif
    for (auto &f : funcmap) {
        // if (f.second->getName().startswith("__renamed__")) {

        f.first->replaceAllUsesWith(UndefValue::get(f.first->getType()));
        f.first->removeFromParent();
        llvm::errs() << "RENAMING\t" << f.second->getName();
        if (accmut_catched_func.find(f.first->getName()) != accmut_catched_func.end())
            f.second->setName("__accmutv2__" + f.first->getName());
#ifdef __APPLE__
        else if (accmut_catched_func_mac_alias.find(f.first->getName()) != accmut_catched_func_mac_alias.end())
            f.second->setName("__accmutv2__" + f.first->getName().substr(2));
#endif
        else
            f.second->setName(f.first->getName());
        llvm::errs() << "\tTO\t" << f.second->getName() << "\n";
        f.first->dropAllReferences();
        /*} else {
            llvm::errs() << "Unknown function name?" << f.second->getName() << "\n";
            exit(-1);
        }*/
    }
}

char RenamePass::ID = 0;
static RegisterPass<RenamePass> X("AccMut-instrument", "AccMut - Rename FILE");