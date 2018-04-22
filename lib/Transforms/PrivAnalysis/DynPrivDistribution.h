// ====------------------- DynPrivDistribution.cpp ---------*- C++ -*----====
//
// This pass dynamically computes the distribution of each privilege set:
// for each privilege set, it dynamically counts the lines of LLVM IR.
//
// ====------------------------------------------------------------------====


#ifndef __DYNPRIVDSTR_H__
#define __DYNPRIVDSTR_H__

#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"


#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "ADT.h"

#define MAIN_FUNC "main"
#define PRIV_REMOVE_FUNC "priv_remove"
#define INIT_COUNT_FUNC "initDynCount"
#define ADD_BB_LOI_FUNC "addBBLOI"
#define ADD_LOI_FUNC "addLOI"
#define REPORT_PRIV_DSTR_FUNC "reportPrivDistr"


namespace llvm {
namespace dynPrivDstr {
struct DynPrivDstr : public ModulePass {
public:
    static char ID;

    // this Module
    Module *m;

    // constructor
    DynPrivDstr();

    virtual bool runOnModule(Module &M);


private:
    // get initial available privilege set 
    uint64_t getInitialPrivSet(Module &M);

    // get basic blocks that have priv_remove call
    void getFuncUserBB(Function *f, std::set<BasicBlock *> &bbs);

    // construct the prototype of reportCount function
    void insertInitDynCountFunc(Module &M);

    // construct the prototype of initDynCount function
    Function *getAddDynCountFunc(Module &M);

    // construct the prototype of addBBLOI function
    Function *getAddBBLOIFunc(Module &M);
    
    // construct the prototype of addLOI function
    Function *getReportPrivDstrFunc(Module &M);

    

    // for debugging purpose
    void print(raw_ostream &O) const;
};  // end of struct DynPrivDstr
}  // end of namespace dynPrivDstr
}  // end of namespace llvm

#endif  // end of define __DYNPRIVDSTR_H__
