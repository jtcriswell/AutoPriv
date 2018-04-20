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
#include "llvm/Support/raw_ostream.h"


#include <map>
#include <cstdint>

#include "ADT.h"
/* #include "LocalAnalysis.h" */
/* #include "PropagateAnalysis.h" */
/* #include "GlobalLiveAnalysis.h" */

#define PRIV_REMOVE "priv_remove"

namespace llvm {
namespace dynPrivDstr {
struct DynPrivDstr : public ModulePass {
public:
    static char ID;

    // this Module
    Module *m;

    DynPrivDstr();

    virtual bool runOnModule(Module &M);

private:
    // for debugging purpose
    void print(raw_ostream &O) const;
};  // end of struct DynPrivDstr
}  // end of namespace dynPrivDstr
}  // end of namespace llvm

#endif  // end of define __DYNPRIVDSTR_H__
