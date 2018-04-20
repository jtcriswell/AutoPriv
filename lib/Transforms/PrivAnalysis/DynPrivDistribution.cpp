// ====------------------- DynPrivDistribution.cpp ---------*- C++ -*----====
//
// This pass dynamically computes the distribution of each privilege set:
// for each privilege set, it dynamically counts the lines of LLVM IR.
//
// This pass will run after -simplifycfg.
//
// ====------------------------------------------------------------------====
//

#include "llvm/IR/Type.h"
/* #include "llvm/Transforms/Scalar/SimplifyCFG.h" */

#include "DynPrivDistribution.h"

using namespace llvm;
using namespace llvm::dynPrivDstr;

// constructor
DynPrivDstr::DynPrivDstr() : ModulePass(ID) {
}

/* void DynPrivDstr::getAnalysisUsage(AnalysisUsage &AU) const { */
/*     AU.addRequired<simplifycfg>(); */
/* } */

bool DynPrivDstr::runOnModule(Module &M) {
    errs() << "Hello from DynPrivDstr Pass!\n";

    m = &M;
    /* print(errs()); */
    return true;  // a transformation pass
}


void DynPrivDstr::print(raw_ostream &O) const {
    for (Module::iterator mi = m->begin(); mi != m->end(); mi++) {
        errs() << mi->getName() << " has " << mi->size() << " basic blocks\n";
        for (Function::iterator fi = mi->begin(); fi != mi->end(); fi ++) {
        }
    }
}


// register this pass
char DynPrivDstr::ID = 0;
static RegisterPass<DynPrivDstr> DPD("DynPrivDstr", "Dynamically count LOC in privilege set.",
                                   true,
                                   false);  // a transformation pass






