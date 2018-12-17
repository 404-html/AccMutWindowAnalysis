//===----------------------------------------------------------------------===//
//
 // This file implements the mutation generator pass
// 
 // Add by Wang Bo. OCT 19, 2015
//
//===----------------------------------------------------------------------===//

#ifndef ACCMUT_MUTATION_GEN_H
#define ACCMUT_MUTATION_GEN_H

#include "llvm/IR/Module.h"

#include <fstream>
#include <string>

using namespace llvm;

class MutationGen: public FunctionPass{
public:
	static char ID;// Pass identification, replacement for typeid
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;

	Module *TheModule;
	static std::ofstream  ofresult; 
	MutationGen(/*Module *M*/);
	virtual bool runOnFunction(Function &F);
	static void genMutationFile(Function & F);
	static std::string getMutationFilePath();
	static bool firstFunctionSeen;
private:
	static void genAOR(Instruction *inst,StringRef fname, int index);
	static void genLOR(Instruction *inst, StringRef fname, int index);
	static void genCOR();
	static void genROR(Instruction *inst,StringRef fname, int index);
	static void genSOR();
	static void genSTDCall(Instruction *inst,StringRef fname, int index);
    static void genSTDStore(Instruction *inst,StringRef fname, int index);
	static void genLVR(Instruction *inst, StringRef fname, int index);
    static void genUOI(Instruction *inst, StringRef fname, int index);
    static void genROV(Instruction *inst, StringRef fname, int index);
    static void genABV(Instruction *inst, StringRef fname, int index);
};


#endif
