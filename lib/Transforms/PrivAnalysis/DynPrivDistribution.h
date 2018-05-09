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
#define EXECVE_FUNC "execve"
#define FORK_FUNC "fork"
#define PRIV_REMOVE_FUNC "priv_remove"
#define INIT_COUNT_FUNC "initDynCount"
#define ADD_BB_LOI_FUNC "addBBLOI"
#define ADD_PRIV_RM_LOI_FUNC "addPrivRmLOI"
#define REPORT_PRIV_DSTR_FUNC "reportPrivDstr"
#define ATEXIT_FUNC "atexit"


namespace llvm {
namespace dynPrivDstr {
struct DynPrivDstr : public ModulePass {
public:
    static char ID;

    // constructor
    DynPrivDstr();

    virtual bool runOnModule(Module &M);

private:
    // get privilege set from a privilege primitive function call
    uint64_t getPrivSetFromPrivPrimitives(CallInst *CI) const;

    // get initial available privilege set 
    uint64_t getInitialPrivSet(Module &M);

    // get basic blocks that have priv_remove call
    void getFuncUserBB(Function *f, std::set<BasicBlock *> &bbs);

    // insert addLOT functions 
    void insertAddLOIFunc(Module &M);

    // construct and insert call to initDynCount function
    void insertInitDynCountFunc(Module &M);

    // construct and insert call to addPrivRmLOI function
    void insertAddPrivRmLOIFunc(Module &M, Instruction *I, uint32_t LOI, uint64_t removedPriv);

    // construct and insert call to addBBLOI function
    void insertAddBBLOIFunc(Module &M, Instruction *insertBefore, uint32_t LOI, StringRef funcName);

    // construct and insert call to atexit function
    void insertAtexitFunc(Module &M);
    
    // construct and insert call to forkHandler function
    void insertInitFunc(Module &M);
    // construct and insert call to forkHandler function
    void insertForkHandler(Module &M, Instruction *insertBefore);

    // a helper method for insertReportPrivDstrFunc and insertAtexitFunc
    Function *getReportPrivDstrFunc(Module &M);

    // for debugging purpose
    void print(raw_ostream &O, Module &M) const;
};  // end of struct DynPrivDstr
}  // end of namespace dynPrivDstr
}  // end of namespace llvm

#endif  // end of define __DYNPRIVDSTR_H__
