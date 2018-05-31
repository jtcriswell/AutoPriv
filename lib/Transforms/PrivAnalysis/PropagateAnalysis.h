// ====----------  PropagateAnalysis.h ----------*- C++ -*---====
//
// Analysis the call graph, propagate the live privilege property
// from callee up to callers.
// The output of the pass is the whole set of the privilege each 
// function requires to run
//
// ====-------------------------------------------------------====

#ifndef __PROPAGTEANALYSIS_H__
#define __PROPAGTEANALYSIS_H__

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"

#include "ADT.h"

#include <vector>
#include <set>

using namespace llvm::privAnalysis;

namespace llvm {
namespace propagateAnalysis {

// PropagateAnalysis class
struct PropagateAnalysis : public ModulePass 
{
public:
    // pass ID
    static char ID;

    // CAPTable after info propagation
    FuncCAPTable_t FuncCAPTable;

    // Data structure for priv capability use in each BB
    // Maps from BB to -> Array of Capabilities
    BBCAPTable_t BBCAPTable;

    // Map from BB to its non-external Function Calls
    BBFuncTable_t BBFuncTable;

    // Dummy Node of callingNode and callsNode
    Function* callingNodeFunc;
    Function* callsNodeFunc;

    // Set of functions called by external code
    std::set<Function *> calledFromExternalCode;

    // constructor
    PropagateAnalysis();

    // get analysis usage
    void getAnalysisUsage(AnalysisUsage &AU) const;

    // do initialization
    virtual bool doInitialization(Module &M);

    // Run on CallGraph SCC
    virtual bool runOnModule(Module &M);

    // remove elements in FuncCAPTable that have no capabilities
    void cleanFuncCAPTable();

    // Print out information for debugging purposes
    void print(raw_ostream &O, const Module *M) const;

    // print out the calling relations in the call graph
    void printCG(CallGraph &CG) const;
    // a helper function for printCG
    void printCallees(CallGraphNode &CGN) const;
private:
    // Insert dummy function
    static Function *InsertDummyFunc(Module &M, const StringRef name); 

    // Data propagation analysis
    void Propagate(Module &M);

};

} // namespace propagateAnalysis
} // namespace llvm

#endif
