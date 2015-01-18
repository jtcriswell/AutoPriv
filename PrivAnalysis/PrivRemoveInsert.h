// ====----------  PrivRemoveInsert.h ------------*- C++ -*---====
//
// Based on the information from GlobalLiveAnalysis, insert 
// privRemove calls at the end of BasicBlocks, to remove 
// unnecessary privileges. 
//
// ====-------------------------------------------------------====

#ifndef __PRIVREMOVEINSERT_H__
#define __PRIVREMOVEINSERT_H__

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"

#include "ADT.h"

#define PRIV_REMOVE_CALL "priv_remove"


using namespace llvm::privAnalysis;

namespace llvm {
namespace privremoveinsert {

struct PrivRemoveInsert : public ModulePass
{
public:
    static char ID;

    PrivRemoveInsert();

    // Initialization
    virtual bool doInitialization(Module &M);

    // Run on Module
    virtual bool runOnModule(Module &M);

    // Preserve analysis usage
    void getAnalysisUsage(AnalysisUsage &AU) const;

private:
    Function *getRemoveCall(Module &M);
};

} // namespace privremoveinsert
} // namespace llvm

#endif
