//===----------------------------------------------------------------------===//
//
// This file decribes the dynamic mutation analysis IR instrumenter pass
//
// Add by Wang Bo. OCT 21, 2015
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/AccMut/DMAInstrumenter.h"
#include "llvm/Transforms/AccMut/MutUtil.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Operator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"

#include<sstream>
#include<set>


using namespace llvm;
using namespace std;

#define ERRMSG(msg) llvm::errs()<<(msg)<<" @ "<<__FILE__<<"->"<<__FUNCTION__<<"():"<<__LINE__<<"\n"

#define VALERRMSG(it, msg, cp) llvm::errs()<<"\tCUR_IT:\t"<<(*(it))<<"\n\t"<<(msg)<<":\t"<<(*(cp))<<"\n"

DMAInstrumenter::DMAInstrumenter(/*Module *M*/) : ModulePass(ID) {
    //this->TheModule = M;
    //getAllMutations();
}
/*

static void test(Function &F){
	for(Function::iterator FI = F.begin(); FI != F.end(); ++FI){
		Function::iterator BB = FI;
		for(BasicBlock::iterator cur_it = BB->begin(); cur_it != BB->end(); ++cur_it){
			#if 0
			if(dyn_cast<CallInst>(&*cur_it)){

				//move all constant literal int to repalce to alloca
				for (auto OI = cur_it->op_begin(), OE = cur_it->op_end(); OI != OE; ++OI){
					if(ConstantInt* cons = dyn_cast<ConstantInt>(&*OI)){
						AllocaInst *alloca = new AllocaInst(cons->getType(), "cons_alias", cur_it);
						StoreInst *str = new StoreInst(cons, alloca, cur_it);
						LoadInst *ld = new LoadInst(alloca, "const_load", cur_it);
						*OI = (Value*) ld;
					}
				}

				Module* TheModule = F.getParent();

				Function* precallfunc = TheModule->getFunction("__accmut__prepare_call");
				std::vector<Value*> params;
				std::stringstream ss;
				ss<<0;
				ConstantInt* from_i32= ConstantInt::get(TheModule->getContext(),
						APInt(32, StringRef(ss.str()), 10));
				params.push_back(from_i32);
				ss.str("");
				ss<<1;
				ConstantInt* to_i32= ConstantInt::get(TheModule->getContext(),
					APInt(32, StringRef(ss.str()), 10));
				params.push_back(to_i32);

				std::bitset<64> signature;
				unsigned char index = 0;
				int record_num = 0;
				//get signature info
				for (auto OI = cur_it->op_begin(), OE = cur_it->op_end(); OI != OE; ++OI, ++index){
					Type* OIType = (dyn_cast<Value>(&*OI))->getType();
					int tp = getTypeMacro(OIType);
					if(tp < 0){
						continue;
					}
					//push type
					ss.str("");
					short tp_and_idx = ((unsigned char)tp)<<8;
					tp_and_idx = tp_and_idx | index;
					ss<<tp_and_idx;
					ConstantInt* ctai = ConstantInt::get(TheModule->getContext(),
						APInt(16, StringRef(ss.str()), 10));
					params.push_back(ctai);
					//now push the idx'th param
					if(LoadInst *ld = dyn_cast<LoadInst>(&*OI)){//is a local var
						params.push_back(ld->getPointerOperand());
					}else{
						llvm::errs()<<"ERROR, ONLY FOR LORDINST @ "<<__FUNCTION__<<" : "<<__LINE__<<"\n";
						exit(0);
					}
					record_num++;
				}
				//insert num of param-records
				ss.str("");
				ss<<record_num;
				ConstantInt* rcd = ConstantInt::get(TheModule->getContext(),
						APInt(32, StringRef(ss.str()), 10));
				params.insert( params.begin()+2 , rcd);

				CallInst *pre = CallInst::Create(precallfunc, params, "", cur_it);

				ConstantInt* zero = ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

				ICmpInst *hasstd = new ICmpInst(cur_it, ICmpInst::ICMP_NE, pre, zero, "hasstd");

				BasicBlock *cur_bb = cur_it->getParent();

				Instruction* oricall = cur_it->clone();

				BasicBlock* label_if_end = cur_bb->splitBasicBlock(cur_it, "if.end");

				BasicBlock* label_if_then = BasicBlock::Create(TheModule->getContext(), "if.then",cur_bb->getParent(), label_if_end);
				BasicBlock* label_if_else = BasicBlock::Create(TheModule->getContext(), "if.else",cur_bb->getParent(), label_if_end);

				cur_bb->back().eraseFromParent();

				BranchInst::Create(label_if_then, label_if_else, hasstd, cur_bb);

				//label_if_then
				label_if_then->getInstList().push_back(oricall);
				BranchInst::Create(label_if_end, label_if_then);

				//label_if_else
				Function *std_handle;
				if(oricall->getType()->isIntegerTy(32)){
					std_handle = TheModule->getFunction("__accmut__std_i32");
				}else if(oricall->getType()->isIntegerTy(64)){
					std_handle = TheModule->getFunction("__accmut__std_i64");
				}else if(oricall->getType()->isVoidTy()){
					std_handle = TheModule->getFunction("__accmut__std_void");
				}else{
					llvm::errs()<<"ERR CALL TYPE @ "<<__FUNCTION__<<" : "<<__LINE__<<"\n";
					exit(0);
				}

				CallInst*  stdcall = CallInst::Create(std_handle, "", label_if_else);
				stdcall->setCallingConv(CallingConv::C);
				stdcall->setTailCall(false);
				AttributeList stdcallPAL;
				stdcall->setAttributes(stdcallPAL);
				BranchInst::Create(label_if_end, label_if_else);

				//label_if_end
				if(oricall->getType()->isVoidTy()){
					cur_it->removeFromParent();
				}
				else{
					PHINode* call_res = PHINode::Create(IntegerType::get(TheModule->getContext(), 32), 2, "call.phi");
					call_res->addIncoming(oricall, label_if_then);
					call_res->addIncoming(stdcall, label_if_else);
					ReplaceInstWithInst(cur_it, call_res);
				}

				return;

			}
			#endif

			if(StoreInst* st = dyn_cast<StoreInst>(&*cur_it)){

				Module* TheModule = F.getParent();

				if(ConstantInt* cons = dyn_cast<ConstantInt>(st->getValueOperand())){
						AllocaInst *alloca = new AllocaInst(cons->getType(), 0, "cons_alias", st);
						*/
/* StoreInst *str = *//*
 new StoreInst(cons, alloca, st);
						LoadInst *ld = new LoadInst(alloca, "const_load", st);
						User::op_iterator OI = st->op_begin();
						*OI = (Value*) ld;
				}

				Function* prestfunc;
				if(st->getValueOperand()->getType()->isIntegerTy(32)){
					prestfunc = TheModule->getFunction("__accmut__prepare_st_i32");
				}
				else if(st->getValueOperand()->getType()->isIntegerTy(64)){
					prestfunc = TheModule->getFunction("__accmut__prepare_st_i64");
				}else{
					ERRMSG("ERR STORE TYPE");
					exit(0);
				}

				std::vector<Value*> params;
				std::stringstream ss;
				ss<<0;
				ConstantInt* from_i32= ConstantInt::get(TheModule->getContext(),
						APInt(32, StringRef(ss.str()), 10));
				params.push_back(from_i32);
				ss.str("");
				ss<<1;
				ConstantInt* to_i32= ConstantInt::get(TheModule->getContext(),
					APInt(32, StringRef(ss.str()), 10));
				params.push_back(to_i32);

				auto OI = st->op_begin() + 1;
				if(LoadInst *ld = dyn_cast<LoadInst>(&*OI)){//is a local var
					params.push_back(ld->getPointerOperand());
				}else if(AllocaInst *alloca = dyn_cast<AllocaInst>(&*OI)){
					params.push_back(alloca);
				}else{
					ERRMSG("NOT A POINTER");
					exit(0);
				}

				CallInst *pre = CallInst::Create(prestfunc, params, "", &*cur_it);

				ConstantInt* zero = ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

				ICmpInst *hasstd = new ICmpInst(&*cur_it, ICmpInst::ICMP_EQ, pre, zero, "hasstd");

				BasicBlock *cur_bb = cur_it->getParent();

				Instruction* orist = cur_it->clone();

				BasicBlock* label_if_end = cur_bb->splitBasicBlock(cur_it, "if.end");

				BasicBlock* label_if_then = BasicBlock::Create(TheModule->getContext(), "if.then",cur_bb->getParent(), label_if_end);
				BasicBlock* label_if_else = BasicBlock::Create(TheModule->getContext(), "if.else",cur_bb->getParent(), label_if_end);

				cur_bb->back().eraseFromParent();

				BranchInst::Create(label_if_then, label_if_else, hasstd, cur_bb);

				//label_if_then
				label_if_then->getInstList().push_back(orist);
				BranchInst::Create(label_if_end, label_if_then);

				//label_if_else
				Function *std_handle = TheModule->getFunction("__accmut__std_store");
				CallInst*  stdcall = CallInst::Create(std_handle, "", label_if_else);
				stdcall->setCallingConv(CallingConv::C);
				stdcall->setTailCall(false);
				AttributeList stdcallPAL;
				stdcall->setAttributes(stdcallPAL);
				BranchInst::Create(label_if_end, label_if_else);

				//label_if_end
				cur_it->eraseFromParent();
				return;
			}
		}
	}
}
*/

bool DMAInstrumenter::runOnModule(Module &M) {
    TheModule = &M;

    MutUtil::getAllMutations(TheModule->getName());

    vector<Mutation *> &AllMutsVec = MutUtil::AllMutsVec;
    if (AllMutsVec.size() == 0)
        return false;

    if (firstTime) {
        StructType *t = TheModule->getTypeByName("struct.Mutation");
        if (t == nullptr) {
            t = StructType::create(
                    TheModule->getContext(),
                    {IntegerType::get(TheModule->getContext(), 32),
                     IntegerType::get(TheModule->getContext(), 32),
                     IntegerType::get(TheModule->getContext(), 32),
                     IntegerType::get(TheModule->getContext(), 64),
                     IntegerType::get(TheModule->getContext(), 64)},
                    "struct.Mutation");
        }
        std::vector<Constant *> vec;
        vec.reserve(AllMutsVec.size());
        for (uint64_t i = 0; i < AllMutsVec.size(); ++i) {
            Mutation *mut = AllMutsVec[i];
            int type = mut->getKind();
            int sop = mut->src_op;
            int op_0 = 0;
            long op_1 = 0;
            long op_2 = 0;
            switch (type) {
                case Mutation::MK_AOR:
                    op_0 = dyn_cast<AORMut>(mut)->tar_op;
                    break;
                case Mutation::MK_LOR:
                    op_0 = dyn_cast<LORMut>(mut)->tar_op;
                    break;
                case Mutation::MK_ROR:
                    op_1 = dyn_cast<RORMut>(mut)->src_pre;
                    op_2 = dyn_cast<RORMut>(mut)->tar_pre;
                    break;
                case Mutation::MK_STD:
                    op_1 = dyn_cast<STDMut>(mut)->func_ty;
                    op_2 = dyn_cast<STDMut>(mut)->retval;
                    break;
                case Mutation::MK_LVR:
                    op_0 = dyn_cast<LVRMut>(mut)->oper_index;
                    op_1 = dyn_cast<LVRMut>(mut)->src_const;
                    op_2 = dyn_cast<LVRMut>(mut)->tar_const;
                    break;
                case Mutation::MK_UOI:
                    op_1 = dyn_cast<UOIMut>(mut)->oper_index;
                    op_2 = dyn_cast<UOIMut>(mut)->ury_tp;
                    break;
                case Mutation::MK_ROV:
                    op_1 = dyn_cast<ROVMut>(mut)->op1;
                    op_2 = dyn_cast<ROVMut>(mut)->op2;
                    break;
                case Mutation::MK_ABV:
                    op_0 = dyn_cast<ABVMut>(mut)->oper_index;
                    break;
                default:
                    break;
            }
            vec.push_back((ConstantStruct::get(t, {
                    ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), type, true),
                    ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), sop, true),
                    ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), op_0, true),
                    ConstantInt::get(IntegerType::get(TheModule->getContext(), 64), op_1, true),
                    ConstantInt::get(IntegerType::get(TheModule->getContext(), 64), op_2, true)
            })));
        }
        ConstantArray *a = dyn_cast<ConstantArray>(ConstantArray::get(ArrayType::get(t, vec.size()),
                                                                      ArrayRef<Constant *>(vec)));
        GlobalVariable *gv1 = new GlobalVariable(
                *TheModule,
                ArrayType::get(t, vec.size()),
                false, llvm::GlobalValue::LinkageTypes::InternalLinkage,
                a,
                "mutarray",
                nullptr,
                llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
                0
        );

        regmutinfo = TheModule->getTypeByName("struct.RegMutInfo");
        if (regmutinfo == nullptr) {
            regmutinfo = StructType::create(
                    TheModule->getContext(),
                    {PointerType::get(t, 0),
                     IntegerType::get(TheModule->getContext(), 32),
                     IntegerType::get(TheModule->getContext(), 32),
                     IntegerType::get(TheModule->getContext(), 32)},
                    "struct.RegMutInfo"
            );
        }

        Constant *gep = ConstantExpr::getInBoundsGetElementPtr(nullptr, gv1, ArrayRef<Constant *>({
                                                                                                          ConstantInt::get(
                                                                                                                  IntegerType::get(
                                                                                                                          TheModule->getContext(),
                                                                                                                          32),
                                                                                                                  0,
                                                                                                                  false),
                                                                                                          ConstantInt::get(
                                                                                                                  IntegerType::get(
                                                                                                                          TheModule->getContext(),
                                                                                                                          32),
                                                                                                                  0,
                                                                                                                  false),
                                                                                                  }));
        ConstantStruct *rmi = dyn_cast<ConstantStruct>(ConstantStruct::get(regmutinfo, {
                gep,
                ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), vec.size(), false),
                ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), 0, false),
                ConstantInt::get(IntegerType::get(TheModule->getContext(), 32), 0, false)
        }));
        rmigv = new GlobalVariable(
                *TheModule,
                regmutinfo,
                false, llvm::GlobalValue::LinkageTypes::InternalLinkage,
                rmi,
                "mutrmi",
                nullptr,
                llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
                0
        );
        firstTime = false;
    }

    bool changed = false;
    for (Function &F : M)
        if (!F.isDeclaration())
            changed |= runOnFunction(F);
    return changed;

}

bool DMAInstrumenter::runOnFunction(Function &F) {
    if (F.getName().startswith("__accmut__")) {
        return false;
    }
    if (F.getName().equals("main")) {
        return true;
    }

    vector<Mutation *> *v = MutUtil::AllMutsMap[F.getName()];

    if (v == NULL || v->size() == 0) {
        return false;
    }

    errs() << "\n######## DMA INSTRUMTNTING MUT  @" << TheModule->getName() << "->" << F.getName()
           << "()  ########\n\n";

    // moveLiteralForFunc(F, v);

    collectCanMove(F, v);
    instrument(F, v);
    //test(F);

    return true;
}


//TYPE BITS OF SIGNATURE
#define CHAR_TP 0
#define SHORT_TP 1
#define INT_TP 2
#define LONG_TP 3

static int getTypeMacro(Type *t) {
    int res = -1;
    if (t->isIntegerTy()) {
        unsigned width = t->getIntegerBitWidth();
        switch (width) {
            case 8:
                res = CHAR_TP;
                break;
            case 16:
                res = SHORT_TP;
                break;
            case 32:
                res = INT_TP;
                break;
            case 64:
                res = LONG_TP;
                break;
            default:
                ERRMSG("OMMITNG PARAM TYPE");
                llvm::errs() << *t << '\n';
                //exit(0);
        }
    }
    return res;
}

static bool isHandledCoveInst(Instruction *inst) {
    return inst->getOpcode() == Instruction::Trunc
           || inst->getOpcode() == Instruction::ZExt
           || inst->getOpcode() == Instruction::SExt
           || inst->getOpcode() == Instruction::BitCast;
    // TODO:: PtrToInt, IntToPtr, AddrSpaceCast and float related Inst are not handled
}

static bool pushPreparecallParam(std::vector<Value *> &params, int index, Value *OI, Module *TheModule) {
    Type *OIType = (dyn_cast<Value>(&*OI))->getType();
    int tp = getTypeMacro(OIType);
    if (tp < 0) {
        return false;
    }

    std::stringstream ss;
    //push type
    ss.str("");
    unsigned short tp_and_idx = ((unsigned short) tp) << 8;
    tp_and_idx = tp_and_idx | (unsigned short) index;
    ss << tp_and_idx;
    ConstantInt *c_t_a_i = ConstantInt::get(TheModule->getContext(),
                                            APInt(16, StringRef(ss.str()), 10));

    //now push the pointer of idx'th param
    if (LoadInst *ld = dyn_cast<LoadInst>(&*OI)) {//is a local var
        Value *ptr_of_ld = ld->getPointerOperand();
        //if the pointer of loadInst dose not point to an integer
        if (SequentialType *t = dyn_cast<SequentialType>(ptr_of_ld->getType())) {
            if (!t->getElementType()->isIntegerTy()) {// TODO: for i32**
                ERRMSG("WARNNING ! Trying to push a none-i32* !! ");
                return false;
            }
        }
        params.push_back(c_t_a_i);
        params.push_back(ptr_of_ld);
    } else if (AllocaInst *alloca = dyn_cast<AllocaInst>(&*OI)) {// a param of the F, fetch it by alloca
        params.push_back(c_t_a_i);
        params.push_back(alloca);
    } else if (GetElementPtrInst *ge = dyn_cast<GetElementPtrInst>(&*OI)) {
        // TODO: test
        // return false;
        params.push_back(c_t_a_i);
        params.push_back(ge);
    } /*else if (Argument *argu = dyn_cast<Argument>(&*OI)) {
    // return false;
		params.push_back(c_t_a_i);
		params.push_back(argu);
    }*/
        // TODO:: for Global Pointer ?!
    else {
        ERRMSG("CAN NOT GET A POINTER");
        Value *v = dyn_cast<Value>(&*OI);
        llvm::errs() << "\tCUR_OPREAND:\t";
        llvm::errs() << *v << "\n";
        // v->dump();
        exit(0);
    }
    return true;
}

static StoreInst *mayAlias(AliasAnalysis *AA, LoadInst *ld, StoreInst *st) {
    auto loc = MemoryLocation::get(ld);
    auto info = AA->getModRefInfo(st, loc);
    if (isNoModRef(info)) {
        return nullptr;
    }
    return st;
}

static StoreInst *mayAliasBetween(AliasAnalysis *AA, LoadInst *ld, CallInst *call) {
    assert(ld->getParent() == call->getParent());
    auto i = ld->getNextNode();
    llvm::errs() << "ld inst" << *ld << "\n";
    while (i != call) {
        if (StoreInst *st = dyn_cast<StoreInst>(i)) {
            llvm::errs() << "store inst" << *st << "\n";
            if (mayAlias(AA, ld, st)) {
                llvm::errs() << "may alias" << *st << "\n";
                return st;
            }
        }
        i = i->getNextNode();
    }
    return nullptr;
}

void DMAInstrumenter::collectCanMove(Function &F, vector<Mutation *> *v) {
    canMove.clear();
    std::unordered_map<Value *, int> numDirectUse;
    // auto &t = getAnalysis<MemoryDependenceWrapperPass>();
    AliasAnalysis *AA = &getAnalysis<AAResultsWrapperPass>(F).getAAResults();
    // If more users exists, don't move
    for (auto i = F.begin(); i != F.end(); ++i) {
        for (auto j = i->begin(); j != i->end(); ++j) {
            for (auto x = j->op_begin(); x != j->op_end(); ++x) {
                numDirectUse[(x->get())]++;
            }
        }
    }
    for (unsigned i = 0; i < v->size(); ++i) {
        int cur_index = (*v)[i]->index;
        for (unsigned j = i + 1; j < v->size(); ++j) {
            if ((*v)[j]->index == cur_index) {
                i = j;
            } else {
                break;
            }
        }
        auto cur_it = getLocation(F, 0, cur_index);
        if (CallInst *inst = dyn_cast<CallInst>(&*cur_it)) {
            for (auto i = inst->op_begin(); i != inst->op_end(); ++i) {
                auto j = i->get();
                if (LoadInst *ld = dyn_cast<LoadInst>(&*j)) {
                    if (numDirectUse[&*ld] != 1) {
                        llvm::errs() << "No\t" << *ld << "\n";
                        canMove[&*ld] = false;
                        continue;
                    } else {
                        // only consider basic block local ops, perhaps all ops are basic block local since we perform no optimize
                        if (ld->getParent() != inst->getParent()) {
                            llvm::errs() << "Not BB\t" << *ld << "\n";
                            canMove[&*ld] = false;
                            continue;
                        }
                        if (StoreInst *st = mayAliasBetween(AA, ld, inst)) {
                            llvm::errs() << "May alias\t" << *ld << "\n" << "With\t" << *st << "\n";
                            canMove[&*ld] = false;
                            continue;
                        }
                        llvm::errs() << "Yes\t" << *ld << "\n";
                        canMove[&*ld] = true;
                        continue;
                    }
                } else {
                    assert(false);
                }
            }
        }
    }

    /*
    for (auto i = F.begin(); i != F.end(); ++i) {
      for (auto j = i->begin(); j != i->end(); ++j) {
        if (isa<LoadInst>(&*j) ||
            isa<Constant>(&*j) ||
            isa<GetElementPtrInst>(*&*j)) {
          if (numDirectUse[&*j] != 1) {
            goto no;
          }
          goto yes;
        } else if (isa<Operator>(&*j)) {
          llvm::errs() << "operator\n";
          goto noneed;
        } else {
          llvm::errs() << "instruction\n";
          Instruction *coversion = dyn_cast<Instruction>(&*j);
          if (isHandledCoveInst(coversion)) {
            Instruction* op_of_cov = dyn_cast<Instruction>(coversion->getOperand(0));
            if(isa<LoadInst>(&*op_of_cov) || isa<AllocaInst>(&*op_of_cov)){
              if (numDirectUse[&*j] != 1 || numDirectUse[&*op_of_cov] != 1) {
                goto no;
              } else {
                goto yes;
              }
            } else {
              llvm::errs() << "noneed inst\n";
              goto noneed;
            }
          } else {
            goto noneed;
          }
        }

      noneed:
        llvm::errs() << "No need\t" << *j << "\n";
        continue;
      no:
        llvm::errs() << "No\t" << *j << "\n";
        canMove[&*j] = false;
        continue;
      yes:
        llvm::errs() << "Yes\t" << *j << "\n";
        canMove[&*j] = true;
        continue;
      }
      }*/
}

void DMAInstrumenter::moveLiteralForFunc(Function &F, vector<Mutation *> *v) {
    int instrumented_insts = 0;
    BasicBlock::iterator cur_it;
    for (unsigned i = 0; i < v->size(); ++i) {
        int cur_index = (*v)[i]->index;
        for (unsigned j = i + 1; j < v->size(); ++j) {
            if ((*v)[j]->index == cur_index) {
                i = j;
            } else {
                break;
            }
        }
        cur_it = getLocation(F, instrumented_insts, cur_index);

        if (dyn_cast<CallInst>(&*cur_it)) {
            //move all constant literal and SSA value to repalce to alloca, e.g foo(a+5)->b = a+5;foo(b)
            for (auto OI = cur_it->op_begin(), OE = cur_it->op_end(); OI != OE; ++OI) {
                if (ConstantInt *cons = dyn_cast<ConstantInt>(&*OI)) {
                    AllocaInst *alloca = new AllocaInst(cons->getType(), 0, (cons->getName().str() + ".alias"),
                                                        &*cur_it);
                    /*StoreInst *str = */new StoreInst(cons, alloca, &*cur_it);
                    LoadInst *ld = new LoadInst(alloca, (cons->getName().str() + ".ld"), &*cur_it);
                    *OI = (Value *) ld;
                    instrumented_insts += 3;//add 'alloca', 'store' and 'load'
                } else if (Instruction *oinst = dyn_cast<Instruction>(&*OI)) {
                    if (oinst->isBinaryOp() ||
                        (oinst->getOpcode() == Instruction::Call) ||
                        isHandledCoveInst(oinst) ||
                        (oinst->getOpcode() == Instruction::PHI) ||
                        (oinst->getOpcode() == Instruction::Select)) {
                        AllocaInst *alloca = new AllocaInst(oinst->getType(), 0, (oinst->getName().str() + ".ptr"),
                                                            &*cur_it);
                        /*StoreInst *str = */new StoreInst(oinst, alloca, &*cur_it);
                        LoadInst *ld = new LoadInst(alloca, (oinst->getName().str() + ".ld"), &*cur_it);
                        *OI = (Value *) ld;
                        instrumented_insts += 3;
                    }
                }
            }
        } else if (StoreInst *st = dyn_cast<StoreInst>(&*cur_it)) {
            if (ConstantInt *cons = dyn_cast<ConstantInt>(st->getValueOperand())) {
                AllocaInst *alloca = new AllocaInst(cons->getType(), 0, "cons_alias", st);
                StoreInst *str = new StoreInst(cons, alloca, st);
                LoadInst *ld = new LoadInst(alloca, "const_load", st);
                User::op_iterator OI = st->op_begin();
                *OI = (Value *) ld;
                instrumented_insts += 3;
            }
        }
    }
}

void DMAInstrumenter::instrument(Function &F, vector<Mutation *> *v) {

    int instrumented_insts = 0;

    //BasicBlock *cur_bb;
    BasicBlock::iterator cur_it;

    for (unsigned i = 0; i < v->size(); i++) {
        vector<Mutation *> tmp;
        Mutation *m = (*v)[i];
        tmp.push_back(m);
        //collect all mutations of one instruction
        for (unsigned j = i + 1; j < v->size(); j++) {
            Mutation *m1 = (*v)[j];
            if (m1->index == m->index) {
                tmp.push_back(m1);
                i = j;        // TODO: check
            } else {
                break;
            }
        }

        cur_it = getLocation(F, instrumented_insts, tmp[0]->index);

        //cur_bb = cur_it->getParent();

        int mut_from, mut_to;
        mut_from = tmp.front()->id;
        mut_to = tmp.back()->id;

        llvm::errs() << "CUR_INST: " << tmp.front()->index
                     << "  (FROM:" << mut_from << "  TO:" << mut_to << ")\t"
                     << *cur_it << "\n";

        if (tmp.size() >= MAX_MUT_NUM_PER_LOCATION) {
            ERRMSG("TOO MANY MUTS ");
            exit(0);
        }

        if (dyn_cast<CallInst>(&*cur_it)) {

            //move all constant literal and SSA value to repalce to alloca, e.g foo(a+5)->b = a+5;foo(b)
            for (auto OI = cur_it->op_begin(), OE = cur_it->op_end(); OI != OE; ++OI) {
                if (ConstantInt *cons = dyn_cast<ConstantInt>(&*OI)) {
                    AllocaInst *alloca = new AllocaInst(cons->getType(), 0, (cons->getName().str() + ".alias"),
                                                        &*cur_it);
                    StoreInst *str = new StoreInst(cons, alloca, &*cur_it);
                    LoadInst *ld = new LoadInst(alloca, (cons->getName().str() + ".ld"), &*cur_it);
                    canMove[ld] = true;
                    *OI = (Value *) ld;
                    instrumented_insts += 3;//add 'alloca', 'store' and 'load'
                } else if (Instruction *oinst = dyn_cast<Instruction>(&*OI)) {
                    if (oinst->isBinaryOp() ||
                        (oinst->getOpcode() == Instruction::Call) ||
                        isHandledCoveInst(oinst) ||
                        (oinst->getOpcode() == Instruction::PHI) ||
                        (oinst->getOpcode() == Instruction::Select)) {
                        AllocaInst *alloca = new AllocaInst(oinst->getType(), 0, (oinst->getName().str() + ".ptr"),
                                                            &*cur_it);
                        StoreInst *str = new StoreInst(oinst, alloca, &*cur_it);
                        LoadInst *ld = new LoadInst(alloca, (oinst->getName().str() + ".ld"), &*cur_it);
                        canMove[ld] = true;
                        *OI = (Value *) ld;
                        instrumented_insts += 3;

                    }
                } else if (Argument *argu = dyn_cast<Argument>(&*OI)) {
                    AllocaInst *alloca = new AllocaInst(argu->getType(), 0, (argu->getName().str() + ".alias"),
                                                        &*cur_it);
                    StoreInst *str = new StoreInst(argu, alloca, &*cur_it);
                    LoadInst *ld = new LoadInst(alloca, (argu->getName().str() + ".ld"), &*cur_it);
                    canMove[ld] = true;
                    *OI = (Value *) ld;
                    instrumented_insts += 3;
                }

            }

            Function *precallfunc = TheModule->getFunction("__accmut__prepare_call");

            if (!precallfunc) {
                std::vector<Type *> FuncTy_4_args;
                FuncTy_4_args.push_back(PointerType::get(regmutinfo, 0));
                FuncTy_4_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                FuncTy_4_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                FuncTy_4_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                FunctionType *FuncTy_4 = FunctionType::get(
                        /*Result=*/IntegerType::get(TheModule->getContext(), 32),
                        /*Params=*/FuncTy_4_args,
                        /*isVarArg=*/true);
                precallfunc = Function::Create(
                        /*Type=*/FuncTy_4,
                        /*Linkage=*/GlobalValue::ExternalLinkage,
                        /*Name=*/"__accmut__prepare_call", TheModule); // (external, no body)
                precallfunc->setCallingConv(CallingConv::C);
            }
            AttributeList func___accmut__prepare_call_PAL;
            {
                SmallVector<AttributeList, 4> Attrs;
                AttributeList PAS;
                {
                    AttrBuilder B;
                    PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                }

                Attrs.push_back(PAS);
                func___accmut__prepare_call_PAL = AttributeList::get(TheModule->getContext(), Attrs);

            }
            precallfunc->setAttributes(func___accmut__prepare_call_PAL);


            std::vector<Value *> params;
            params.push_back(rmigv);
            std::stringstream ss;
            ss << mut_from;
            ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(),
                                                     APInt(32, StringRef(ss.str()), 10));
            params.push_back(from_i32);
            ss.str("");
            ss << mut_to;
            ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                                   APInt(32, StringRef(ss.str()), 10));
            params.push_back(to_i32);

            int index = 0;
            int record_num = 0;
            std::vector<int> pushed_param_idx;

            //get signature info
            for (auto OI = cur_it->op_begin(), OE = cur_it->op_end() - 1; OI != OE; ++OI, ++index) {

                Value *v = dyn_cast<Value>(&*OI);
                bool succ = pushPreparecallParam(params, index, v, TheModule);

                if (succ) {
                    pushed_param_idx.push_back(index);
                    record_num++;
                } else {
                    ERRMSG("---- WARNNING : PUSH PARAM FAILURE ");
                    VALERRMSG(cur_it, "CUR_OPRAND", v);
                }

            }

            if (!pushed_param_idx.empty()) {
                llvm::errs() << "---- PUSH PARAM : ";
                for (auto it = pushed_param_idx.begin(), ie = pushed_param_idx.end(); it != ie; ++it) {
                    llvm::errs() << *it << "'th\t";
                }
                llvm::errs() << "\n";
                llvm::errs() << *cur_it << "\n";
            }

            //insert num of param-records
            ss.str("");
            ss << record_num;
            ConstantInt *rcd = ConstantInt::get(TheModule->getContext(),
                                                APInt(32, StringRef(ss.str()), 10));
            params.insert(params.begin() + 3, rcd);

            CallInst *pre = CallInst::Create(precallfunc, params, "", &*cur_it);
            pre->setCallingConv(CallingConv::C);
            pre->setTailCall(false);
            AttributeList preattrset;
            pre->setAttributes(preattrset);

            ConstantInt *zero = ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

            ICmpInst *hasstd = new ICmpInst(&*cur_it, ICmpInst::ICMP_EQ, pre, zero, "hasstd.call");

            BasicBlock *cur_bb = cur_it->getParent();

            Instruction *oricall = cur_it->clone();

            BasicBlock *label_if_end = cur_bb->splitBasicBlock(cur_it, "stdcall.if.end");

            BasicBlock *label_if_then = BasicBlock::Create(TheModule->getContext(), "stdcall.if.then",
                                                           cur_bb->getParent(), label_if_end);
            BasicBlock *label_if_else = BasicBlock::Create(TheModule->getContext(), "stdcall.if.else",
                                                           cur_bb->getParent(), label_if_end);

            cur_bb->back().eraseFromParent();

            BranchInst::Create(label_if_then, label_if_else, hasstd, cur_bb);


            //label_if_then
            //move the loadinsts of params into if_then_block
            index = 0;
            for (auto OI = oricall->op_begin(), OE = oricall->op_end() - 1; OI != OE; ++OI, ++index) {
                continue;

                //only move pushed parameters
                if (find(pushed_param_idx.begin(), pushed_param_idx.end(), index) == pushed_param_idx.end()) {
                    continue;
                }
                if (Argument *arg = dyn_cast<Argument>(&*OI)) {
                    llvm::errs() << "Can't move arguments: " << *(OI->get()) << "\n";
                    continue;
                }
                if (canMove.find(OI->get()) == canMove.end()) {
                    llvm::errs() << "DON'T KNOW IF IT CAN MOVE\t" << *(OI->get()) << "\n";
                    exit(-1);
                }
                if (canMove[OI->get()] == false) {
                    llvm::errs() << "Can move == false: " << *(OI->get()) << "\n";
                    continue;
                }

                if (LoadInst *ld = dyn_cast<LoadInst>(&*OI)) {
                    ld->removeFromParent();
                    label_if_then->getInstList().push_back(ld);
                    // auto newld = ld->clone();
                    // OI->set(newld);
                    // label_if_then->getInstList().push_back(newld);
                    // instrumented_insts++;
                } else if (
                    // Constant *con =
                        dyn_cast<Constant>(&*OI)) {
                    // TODO::  test
                    continue;
                } else if (GetElementPtrInst *ge = dyn_cast<GetElementPtrInst>(&*OI)) {
                    // TODO: test
                    ge->removeFromParent();
                    label_if_then->getInstList().push_back(ge);
                    // auto newge = ge->clone();
                    // OI->set(newge);
                    // label_if_then->getInstList().push_back(newge);
                    // instrumented_insts++;
                } else if (Operator *op = dyn_cast<Operator>(&*OI)) {
                    ERRMSG("Operator");
                    exit(0);
                } else {
                    // TODO:: check
                    // TODO:: instrumented_insts !!!
                    Instruction *coversion = dyn_cast<Instruction>(&*OI);
                    // coversion->dump();
                    if (isHandledCoveInst(coversion)) {
                        Instruction *op_of_cov = dyn_cast<Instruction>(coversion->getOperand(0));
                        if (dyn_cast<LoadInst>(&*op_of_cov) || dyn_cast<AllocaInst>(&*op_of_cov)) {
                            op_of_cov->removeFromParent();
                            label_if_then->getInstList().push_back(op_of_cov);
                            coversion->removeFromParent();
                            label_if_then->getInstList().push_back(coversion);
                            // auto new_op_of_cov = op_of_cov->clone();
                            // auto new_coversion = coversion->clone();
                            // new_coversion->setOperand(0, new_op_of_cov);
                            // OI->set(new_coversion);
                            // label_if_then->getInstList().push_back(new_op_of_cov);
                            // label_if_then->getInstList().push_back(new_coversion);
                            // instrumented_insts+=2;
                        } else {
                            ERRMSG("CAN MOVE GET A POINTER INTO IF.THEN");
                            Value *v = dyn_cast<Value>(&*OI);
                            VALERRMSG(cur_it, "CUR_OPRAND", v);
                            exit(0);
                        }
                    } else {
                        ERRMSG("CAN MOVE GET A POINTER INTO IF.THEN");
                        Value *v = dyn_cast<Value>(&*OI);
                        VALERRMSG(cur_it, "CUR_OPRAND", v);
                        exit(0);
                    }
                }
            }
            label_if_then->getInstList().push_back(oricall);
            BranchInst::Create(label_if_end, label_if_then);

            //label_if_else
            Function *std_handle;
            if (oricall->getType()->isIntegerTy(32)) {
                std_handle = TheModule->getFunction("__accmut__stdcall_i32");

                if (!std_handle) {
                    std::vector<Type *> FuncTy_0_args;
                    FunctionType *FuncTy_0 = FunctionType::get(
                            /*Result=*/IntegerType::get(TheModule->getContext(), 32),
                            /*Params=*/FuncTy_0_args,
                            /*isVarArg=*/false);
                    std_handle = Function::Create(
                            /*Type=*/FuncTy_0,
                            /*Linkage=*/GlobalValue::ExternalLinkage,
                            /*Name=*/"__accmut__stdcall_i32", TheModule); // (external, no body)
                    std_handle->setCallingConv(CallingConv::C);
                }
                AttributeList func___accmut__stdcall_i32_PAL;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    func___accmut__stdcall_i32_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                }
                std_handle->setAttributes(func___accmut__stdcall_i32_PAL);


            } else if (oricall->getType()->isIntegerTy(64)) {
                std_handle = TheModule->getFunction("__accmut__stdcall_i64");
                if (!std_handle) {
                    std::vector<Type *> FuncTy_6_args;
                    FunctionType *FuncTy_6 = FunctionType::get(
                            /*Result=*/IntegerType::get(TheModule->getContext(), 64),
                            /*Params=*/FuncTy_6_args,
                            /*isVarArg=*/false);
                    std_handle = Function::Create(
                            /*Type=*/FuncTy_6,
                            /*Linkage=*/GlobalValue::ExternalLinkage,
                            /*Name=*/"__accmut__stdcall_i64", TheModule); // (external, no body)
                    std_handle->setCallingConv(CallingConv::C);
                }
                AttributeList func___accmut__stdcall_i64_PAL;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    func___accmut__stdcall_i64_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                }
                std_handle->setAttributes(func___accmut__stdcall_i64_PAL);


            } else if (oricall->getType()->isVoidTy()) {
                std_handle = TheModule->getFunction("__accmut__stdcall_void");
                if (!std_handle) {

                    std::vector<Type *> FuncTy_8_args;
                    FunctionType *FuncTy_8 = FunctionType::get(
                            /*Result=*/Type::getVoidTy(TheModule->getContext()),
                            /*Params=*/FuncTy_8_args,
                            /*isVarArg=*/false);

                    std_handle = Function::Create(
                            /*Type=*/FuncTy_8,
                            /*Linkage=*/GlobalValue::ExternalLinkage,
                            /*Name=*/"__accmut__stdcall_void", TheModule); // (external, no body)
                    std_handle->setCallingConv(CallingConv::C);
                }
                AttributeList func___accmut__stdcall_void_PAL;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    func___accmut__stdcall_void_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                }
                std_handle->setAttributes(func___accmut__stdcall_void_PAL);

            } else {
                ERRMSG("ERR CALL TYPE ");
                // oricall->dump();
                // oricall->getType()->dump();
                exit(0);
            }//}else if(oricall->getType()->isPointerTy()){


            CallInst *stdcall = CallInst::Create(std_handle, "", label_if_else);
            stdcall->setCallingConv(CallingConv::C);
            stdcall->setTailCall(false);
            AttributeList stdcallPAL;
            stdcall->setAttributes(stdcallPAL);
            BranchInst::Create(label_if_end, label_if_else);

            //label_if_end
            if (oricall->getType()->isVoidTy()) {
                cur_it->eraseFromParent();
                instrumented_insts += 6;
            } else {
                PHINode *call_res = PHINode::Create(oricall->getType(), 2, "call.phi");
                call_res->addIncoming(oricall, label_if_then);
                call_res->addIncoming(stdcall, label_if_else);
                ReplaceInstWithInst(&*cur_it, call_res);
                instrumented_insts += 7;
            }

        } else if (StoreInst *st = dyn_cast<StoreInst>(&*cur_it)) {

            // TODO:: add or call inst?
            if (ConstantInt *cons = dyn_cast<ConstantInt>(st->getValueOperand())) {
                AllocaInst *alloca = new AllocaInst(cons->getType(), 0, "cons_alias", st);
                StoreInst *str = new StoreInst(cons, alloca, st);
                LoadInst *ld = new LoadInst(alloca, "const_load", st);
                User::op_iterator OI = st->op_begin();
                *OI = (Value *) ld;
                instrumented_insts += 3;
            }

            Function *prestfunc;
            if (st->getValueOperand()->getType()->isIntegerTy(32)) {
                prestfunc = TheModule->getFunction("__accmut__prepare_st_i32");
                if (!prestfunc) {
                    PointerType *PointerTy_1 = PointerType::get(IntegerType::get(TheModule->getContext(), 32), 0);
                    std::vector<Type *> FuncTy_5_args;
                    FuncTy_5_args.push_back(PointerType::get(regmutinfo, 0));
                    FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    FuncTy_5_args.push_back(PointerTy_1);
                    FunctionType *f_tp = FunctionType::get(IntegerType::get(TheModule->getContext(), 32),
                                                           FuncTy_5_args, false);
                    prestfunc = Function::Create(f_tp, GlobalValue::ExternalLinkage,
                                                 "__accmut__prepare_st_i32", TheModule); // (external, no body)
                    prestfunc->setCallingConv(CallingConv::C);
                }

                AttributeList prestfunc_PAL;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    prestfunc_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                }
                prestfunc->setAttributes(prestfunc_PAL);

            } else if (st->getValueOperand()->getType()->isIntegerTy(64)) {
                prestfunc = TheModule->getFunction("__accmut__prepare_st_i64");
                if (!prestfunc) {

                    PointerType *PointerTy_2 = PointerType::get(IntegerType::get(TheModule->getContext(), 64), 0);

                    std::vector<Type *> FuncTy_6_args;
                    FuncTy_6_args.push_back(PointerType::get(regmutinfo, 0));
                    FuncTy_6_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    FuncTy_6_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    FuncTy_6_args.push_back(IntegerType::get(TheModule->getContext(), 64));
                    FuncTy_6_args.push_back(PointerTy_2);
                    FunctionType *FuncTy_6 = FunctionType::get(IntegerType::get(TheModule->getContext(), 32),
                                                               FuncTy_6_args, false);


                    prestfunc = Function::Create(FuncTy_6, GlobalValue::ExternalLinkage,
                                                 "__accmut__prepare_st_i64", TheModule);
                    prestfunc->setCallingConv(CallingConv::C);
                }

                AttributeList pal;
                {
                    SmallVector<AttributeList, 4> Attrs;
                    AttributeList PAS;
                    {
                        AttrBuilder B;
                        PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                    }

                    Attrs.push_back(PAS);
                    pal = AttributeList::get(TheModule->getContext(), Attrs);

                }
                prestfunc->setAttributes(pal);


            } else {
                ERRMSG("ERR STORE TYPE ");
                VALERRMSG(cur_it, "CUR_TYPE", st->getValueOperand()->getType());
                exit(0);
            }


            std::vector<Value *> params;
            params.push_back(rmigv);
            std::stringstream ss;
            ss << mut_from;
            ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(),
                                                     APInt(32, StringRef(ss.str()), 10));
            params.push_back(from_i32);
            ss.str("");
            ss << mut_to;
            ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                                   APInt(32, StringRef(ss.str()), 10));
            params.push_back(to_i32);

            Value *tobestored = dyn_cast<Value>(st->op_begin());
            params.push_back(tobestored);

            auto addr = st->op_begin() + 1;// the pointer of the storeinst
            if (LoadInst *ld = dyn_cast<LoadInst>(&*addr)) {//is a local var
                params.push_back(ld);
            } else if (AllocaInst *alloca = dyn_cast<AllocaInst>(&*addr)) {
                params.push_back(alloca);
            } else if (Constant *con = dyn_cast<Constant>(&*addr)) {
                params.push_back(con);
            } else if (GetElementPtrInst *gete = dyn_cast<GetElementPtrInst>(&*addr)) {
                params.push_back(gete);
            } else {
                ERRMSG("NOT A POINTER ");
                // cur_it->dump();
                Value *v = dyn_cast<Value>(&*addr);
                // v->dump();
                exit(0);
            }
            CallInst *pre = CallInst::Create(prestfunc, params, "", &*cur_it);
            pre->setCallingConv(CallingConv::C);
            pre->setTailCall(false);
            AttributeList attrset;
            pre->setAttributes(attrset);

            ConstantInt *zero = ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

            ICmpInst *hasstd = new ICmpInst(&*cur_it, ICmpInst::ICMP_EQ, pre, zero, "hasstd.st");

            BasicBlock *cur_bb = cur_it->getParent();

            BasicBlock *label_if_end = cur_bb->splitBasicBlock(cur_it, "stdst.if.end");

            BasicBlock *label_if_else = BasicBlock::Create(TheModule->getContext(), "std.st", cur_bb->getParent(),
                                                           label_if_end);

            cur_bb->back().eraseFromParent();

            BranchInst::Create(label_if_end, label_if_else, hasstd, cur_bb);


            //label_if_else
            Function *std_handle = TheModule->getFunction("__accmut__std_store");

            if (!std_handle) {

                std::vector<Type *> args;
                FunctionType *ftp = FunctionType::get(Type::getVoidTy(TheModule->getContext()),
                                                      args, false);
                std_handle = Function::Create(ftp, GlobalValue::ExternalLinkage, "__accmut__std_store",
                                              TheModule); // (external, no body)
                std_handle->setCallingConv(CallingConv::C);
            }
            AttributeList func___accmut__std_store_PAL;
            {
                SmallVector<AttributeList, 4> Attrs;
                AttributeList PAS;
                {
                    AttrBuilder B;
                    PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                }

                Attrs.push_back(PAS);
                func___accmut__std_store_PAL = AttributeList::get(TheModule->getContext(), Attrs);

            }
            std_handle->setAttributes(func___accmut__std_store_PAL);


            CallInst *stdcall = CallInst::Create(std_handle, "", label_if_else);
            stdcall->setCallingConv(CallingConv::C);
            stdcall->setTailCall(false);
            AttributeList stdcallPAL;
            stdcall->setAttributes(stdcallPAL);
            BranchInst::Create(label_if_end, label_if_else);

            //label_if_end
            cur_it->eraseFromParent();
            instrumented_insts += 4;
        } else {
            // FOR ARITH INST
            if (cur_it->getOpcode() >= Instruction::Add &&
                cur_it->getOpcode() <= Instruction::Xor) {
                Type *ori_ty = cur_it->getType();
                Function *f_process;
                if (ori_ty->isIntegerTy(32)) {
                    f_process = TheModule->getFunction("__accmut__process_i32_arith");

                    if (!f_process) {
                        std::vector<Type *> ftp_args;
                        ftp_args.push_back(PointerType::get(regmutinfo, 0));
                        ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FunctionType *ftp = FunctionType::get(IntegerType::get(TheModule->getContext(), 32), ftp_args,
                                                              false);
                        f_process = Function::Create(ftp, GlobalValue::ExternalLinkage,
                                                     "__accmut__process_i32_arith", TheModule); // (external, no body)
                        f_process->setCallingConv(CallingConv::C);
                    }
                    AttributeList func___accmut__process_i32_arith_PAL;
                    {
                        SmallVector<AttributeList, 4> Attrs;
                        AttributeList PAS;
                        {
                            AttrBuilder B;
                            PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                        }

                        Attrs.push_back(PAS);
                        func___accmut__process_i32_arith_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                    }
                    f_process->setAttributes(func___accmut__process_i32_arith_PAL);
                } else if (ori_ty->isIntegerTy(64)) {
                    f_process = TheModule->getFunction("__accmut__process_i64_arith");

                    std::vector<Type *> ftp_args;
                    ftp_args.push_back(PointerType::get(regmutinfo, 0));
                    ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    ftp_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                    ftp_args.push_back(IntegerType::get(TheModule->getContext(), 64));
                    ftp_args.push_back(IntegerType::get(TheModule->getContext(), 64));
                    FunctionType *ftp = FunctionType::get(IntegerType::get(TheModule->getContext(), 64), ftp_args,
                                                          false);
                    if (!f_process) {
                        f_process = Function::Create(ftp, GlobalValue::ExternalLinkage, "__accmut__process_i64_arith",
                                                     TheModule);
                        f_process->setCallingConv(CallingConv::C);
                    }
                    AttributeList pal;
                    {
                        SmallVector<AttributeList, 4> Attrs;
                        AttributeList PAS;
                        {
                            AttrBuilder B;
                            PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                        }

                        Attrs.push_back(PAS);
                        pal = AttributeList::get(TheModule->getContext(), Attrs);

                    }
                    f_process->setAttributes(pal);
                } else {
                    ERRMSG("ArithInst TYPE ERROR ");
                    // cur_it->dump();
                    llvm::errs() << *ori_ty << "\n";
                    // TODO:: handle i1, i8, i64 ... type
                    exit(0);
                }

                std::vector<Value *> int_call_params;
                int_call_params.push_back(rmigv);
                std::stringstream ss;
                ss << mut_from;
                ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef(ss.str()), 10));
                int_call_params.push_back(from_i32);
                ss.str("");
                ss << mut_to;
                ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(), APInt(32, StringRef(ss.str()), 10));
                int_call_params.push_back(to_i32);
                int_call_params.push_back(cur_it->getOperand(0));
                int_call_params.push_back(cur_it->getOperand(1));
                CallInst *call = CallInst::Create(f_process, int_call_params);
                ReplaceInstWithInst(&*cur_it, call);


            }
                // FOR ICMP INST
            else if (cur_it->getOpcode() == Instruction::ICmp) {
                Function *f_process;

                if (cur_it->getOperand(0)->getType()->isIntegerTy(32)) {
                    f_process = TheModule->getFunction("__accmut__process_i32_cmp");

                    if (!f_process) {

                        std::vector<Type *> FuncTy_3_args;
                        FuncTy_3_args.push_back(PointerType::get(regmutinfo, 0));
                        FuncTy_3_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FuncTy_3_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FuncTy_3_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FuncTy_3_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FunctionType *FuncTy_3 = FunctionType::get(
                                /*Result=*/IntegerType::get(TheModule->getContext(), 32),
                                /*Params=*/FuncTy_3_args,
                                /*isVarArg=*/false);

                        f_process = Function::Create(
                                /*Type=*/FuncTy_3,
                                /*Linkage=*/GlobalValue::ExternalLinkage,
                                /*Name=*/"__accmut__process_i32_cmp", TheModule); // (external, no body)
                        f_process->setCallingConv(CallingConv::C);
                    }
                    AttributeList func___accmut__process_i32_cmp_PAL;
                    {
                        SmallVector<AttributeList, 4> Attrs;
                        AttributeList PAS;
                        {
                            AttrBuilder B;
                            PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                        }

                        Attrs.push_back(PAS);
                        func___accmut__process_i32_cmp_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                    }
                    f_process->setAttributes(func___accmut__process_i32_cmp_PAL);

                } else if (cur_it->getOperand(0)->getType()->isIntegerTy(64)) {
                    f_process = TheModule->getFunction("__accmut__process_i64_cmp");

                    if (!f_process) {

                        std::vector<Type *> FuncTy_5_args;
                        FuncTy_5_args.push_back(PointerType::get(regmutinfo, 0));
                        FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 32));
                        FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 64));
                        FuncTy_5_args.push_back(IntegerType::get(TheModule->getContext(), 64));
                        FunctionType *FuncTy_5 = FunctionType::get(
                                /*Result=*/IntegerType::get(TheModule->getContext(), 32),
                                /*Params=*/FuncTy_5_args,
                                /*isVarArg=*/false);


                        f_process = Function::Create(
                                /*Type=*/FuncTy_5,
                                /*Linkage=*/GlobalValue::ExternalLinkage,
                                /*Name=*/"__accmut__process_i64_cmp", TheModule); // (external, no body)
                        f_process->setCallingConv(CallingConv::C);
                    }
                    AttributeList func___accmut__process_i64_cmp_PAL;
                    {
                        SmallVector<AttributeList, 4> Attrs;
                        AttributeList PAS;
                        {
                            AttrBuilder B;
                            PAS = AttributeList::get(TheModule->getContext(), ~0U, B);
                        }

                        Attrs.push_back(PAS);
                        func___accmut__process_i64_cmp_PAL = AttributeList::get(TheModule->getContext(), Attrs);

                    }
                    f_process->setAttributes(func___accmut__process_i64_cmp_PAL);

                } else {
                    ERRMSG("ICMP TYPE ERROR ");
                    exit(0);
                }

                std::vector<Value *> int_call_params;
                int_call_params.push_back(rmigv);
                std::stringstream ss;
                ss << mut_from;
                ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(),
                                                         APInt(32, StringRef(ss.str()), 10));
                int_call_params.push_back(from_i32);
                ss.str("");
                ss << mut_to;
                ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                                       APInt(32, StringRef(ss.str()), 10));
                int_call_params.push_back(to_i32);
                int_call_params.push_back(cur_it->getOperand(0));
                int_call_params.push_back(cur_it->getOperand(1));
                CallInst *call = CallInst::Create(f_process, int_call_params, "", &*cur_it);
                CastInst *i32_conv = new TruncInst(call, IntegerType::get(TheModule->getContext(), 1), "");

                instrumented_insts++;

                ReplaceInstWithInst(&*cur_it, i32_conv);
            }
        }

    }
}

#undef CHAR_TP
#undef SHORT_TP
#undef INT_TP
#undef LONG_TP

BasicBlock::iterator DMAInstrumenter::getLocation(Function &F, int instrumented_insts, int index) {
    int cur = 0;
    for (Function::iterator FI = F.begin(); FI != F.end(); ++FI) {
        Function::iterator BB = FI;
        for (BasicBlock::iterator BI = BB->begin(); BI != BB->end(); ++BI, cur++) {
            if (index + instrumented_insts == cur) {
                return BI;
            }
        }
    }
    return F.back().end();
}

bool DMAInstrumenter::hasMutation(Instruction *inst, vector<Mutation *> *v) {
    int index = 0;
    Function *F = inst->getParent()->getParent();

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I, index++) {
        if ((&*I) == inst) {
            break;
        }
    }

    //errs()<<" ~~~~~ INDEX : "<<index<<"\n";
    for (vector<Mutation *>::iterator it = v->begin(), end = v->end(); it != end; it++) {
        if ((*it)->index == index) {
            return true;
        } else if ((*it)->index > index) {// TODO: check
            return false;
        }
    }
    return false;
}


/*------------------reserved begin-------------------*/
void DMAInstrumenter::getAnalysisUsage(AnalysisUsage &AU) const {
    // AU.setPreservesAll();
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<MemoryDependenceWrapperPass>();
}

char DMAInstrumenter::ID = 0;
static RegisterPass<DMAInstrumenter> X("AccMut-instrument", "AccMut - Instrument Code");
/*-----------------reserved end --------------------*/
