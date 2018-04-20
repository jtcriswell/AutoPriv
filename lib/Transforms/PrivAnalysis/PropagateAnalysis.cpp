// ====----------  PropagateAnalysis.cpp ---------*- C++ -*---====
//
// Analysis the call graph, propagate the live privilege property
// from callee up to callers.
// The output of the pass is the whole set of the privilege each 
// function requires to run
//
// ====-------------------------------------------------------====

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Analysis/CallGraph.h"

#include "PropagateAnalysis.h"
#include "LocalAnalysis.h"
#include "DSAExternAnalysis.h"
// #include "dsa/DataStructure.h"
// #include "dsa/DSGraph.h"
// #include "dsa/CallTargets.h"

#include <array>
#include <vector>
#include <map>
#include <stack>
#include <set>

using namespace llvm;
using namespace dsa;
using namespace llvm::privAnalysis;
using namespace llvm::localAnalysis;
using namespace llvm::propagateAnalysis;
using namespace llvm::dsaexterntarget;

// PropagateAnalysis constructor
PropagateAnalysis::PropagateAnalysis() : ModulePass(ID) { }


// Require Analysis Usage
void PropagateAnalysis::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<LocalAnalysis>();
    AU.addRequired<DSAExternAnalysis>();
    // AU.addRequired<CallTargetFinder<TDDataStructures> >();

    // preserve usage
    AU.setPreservesAll();
}


// Do initialization
bool PropagateAnalysis::doInitialization(Module &M)
{
    return false;
}


// Run on Module
// param: M - the program Module
bool PropagateAnalysis::runOnModule(Module &M)
{
    LocalAnalysis &LA = getAnalysis<LocalAnalysis>();

    // Get all data structures for propagation analysis
    FuncCAPTable = LA.FuncCAPTable;
    BBCAPTable = LA.BBCAPTable;
    BBFuncTable = LA.BBFuncTable;

    Propagate(M);

    return false;
}


// Adding dummy function of type void(*)(void)
// param: M    - the module
//        name - function name
Function* PropagateAnalysis::InsertDummyFunc(Module &M, const StringRef name)
{
    Type *voidTy = Type::getVoidTy(M.getContext());
    std::vector<Type *>Params;
    
    FunctionType *dummyType = FunctionType::get(voidTy, ArrayRef<Type *>(Params),
                                                false);
    Constant *func = M.getOrInsertFunction(name, dummyType);
    return dyn_cast<Function>(func);
}


// Depth First Search data propagation analysis
// param: M - the program module
//        FuncCAPTable - the captable to store live analysis data
void PropagateAnalysis::Propagate(Module &M)
{
    // The ins and outs of function
    FuncCAPTable_t FuncCAPTable_in;
    FuncCAPTable_t FuncCAPTable_out;
    CallGraph CG(M);
    bool ischanged;

    // Get DSA analysis of callgraph
    // const CallTargetFinder<TDDataStructures> &DSAFinder 
    //     = getAnalysis<CallTargetFinder<TDDataStructures> >();
    const DSAExternAnalysis &DSAFinder = getAnalysis<DSAExternAnalysis>();
    FunctionMap_t callgraphMap = DSAFinder.callgraphMap;

    // Add dummy external calls node function as NULL
    // Add them to function table 
    CallGraphNode* callsNode = CG.getCallsExternalNode();
    CallGraphNode* callingNode = CG.getExternalCallingNode();
    callsNodeFunc = InsertDummyFunc(M, "CallsExternNode");
    callingNodeFunc = InsertDummyFunc(M, "CallingExternNode");
    FuncCAPTable[callsNodeFunc] = {};
    FuncCAPTable[callingNodeFunc] = {};

    // copy keys to FuncCAPTable_in
    // TODO: Is FuncCAPTable_in really needed here?
    // TODO: Could be using FuncCAPTable directly?
    CopyTableKeys(FuncCAPTable_in, FuncCAPTable);
    CopyTableKeys(FuncCAPTable_out, FuncCAPTable);



    // Keep iterating until converged
    do {
        ischanged = false;

        //
        // Iterate the whole callgraph.  For each node, propagate information
        // from callees into callers.
        //
        for (CallGraph::iterator NI = CG.begin(), NE = CG.end();
             NI != NE; ++NI) {
            //
            // Get CallgraphNode
            //
            CallGraphNode *N = NI->second;

            //
            // Special handle external nodes: use the dummy function for the
            // external node that calls into the module and ignore the node
            // representing external functions that we call.
            //
            Function *FCaller;
            if (N == callingNode)    { FCaller = callingNodeFunc; }
            else if (N == callsNode) { continue; }
            else                     { FCaller = N->getFunction(); }

            // Get Caller mapped array in FuncCAPTables
            CAPArray_t &callerIn = FuncCAPTable_in[FCaller];
            CAPArray_t &callerOut = FuncCAPTable_out[FCaller];

            //
            // Iterate through Callgraphnode for callees
            // propagate info from callee to caller
            //
            for (CallGraphNode::iterator RI = N->begin(), RE = N->end();
                 RI != RE; ++RI) {
                CallGraphNode* callee = RI->second;
                Function* FCallee = callee->getFunction(); 

                if (callee == callingNode) { continue; }

                // special case main function
                // as no info should propagate from main node
                if (FCallee == M.getFunction("main")) { continue; }

                // Get callee
                // If callee is external callsNode, find it in DSA
                // --------------------------------------------- //
                // For function calling to external callsNode,
                // it indicates that it's calling to unresolved
                // function pointers, and needs info propagated
                // from external callingNode.
                // The only exception being when DSA resolves
                // the function pointers (It's "complete" in DSA
                // analysis), then it could propagate from only 
                // the resolved function pointers
                // --------------------------------------------- //
                if (callee == callsNode) {
                    // Find in DSA. If compelete in DSA, then don't propagate
                    // from extern callsnode 
                    if (callgraphMap.find(FCallee) != callgraphMap.end()) {
                        // propagate from all its callees in DSA analysis
                        std::vector<Function*>DSAcallees
                            = callgraphMap[FCallee];
                        for (std::vector<Function*>::
                                 iterator CI = DSAcallees.begin(), CE = DSAcallees.end();
                             CI!= CE; ++CI) {
                            CAPArray_t &calleeIn = FuncCAPTable_in[*CI];
                            ischanged |= UnionCAPArrays(callerOut, calleeIn);
                        }
                    }
                    else {
                        // incomplete in DSA, propagate from extern calls node
                        CAPArray_t &calleeIn = FuncCAPTable_in[callsNodeFunc];
                        ischanged |= UnionCAPArrays(callerOut, calleeIn);
                    }
                }

                // Propagate all information from callee to caller_out 
                ischanged |= UnionCAPArrays(callerOut, FuncCAPTable_in[FCallee]);
            } // Iterate through Callgraphnode for callees

            // Propagate all information from caller_out to caller_in
            ischanged |= UnionCAPArrays(callerOut, FuncCAPTable[FCaller]);
            ischanged |= UnionCAPArrays(callerIn, callerOut);
        } // iterator for caller nodes

        // special handle calls external node, propagate callees of external
        // calling node to this calls external node
        CAPArray_t &callingNodeIn = FuncCAPTable_in[callingNodeFunc];
        CAPArray_t &callsNodeIn = FuncCAPTable_in[callsNodeFunc];
        CAPArray_t &callsNodeOut = FuncCAPTable_out[callsNodeFunc];
        
        ischanged |= UnionCAPArrays(callsNodeOut, callingNodeIn);
        ischanged |= UnionCAPArrays(callsNodeIn, callsNodeOut);

    } while (ischanged); // main loop


    // Erase dummy function nodes. Restore function-CAPArray table
    // FuncCAPTable_in.erase(callsNodeFunc);
    // FuncCAPTable_in.erase(callingNodeFunc);

    FuncCAPTable = FuncCAPTable_in;
    

    //
    // Record which functions are called by external code.
    //
    for (CallGraphNode::iterator RI = callingNode->begin(),
                                 RE = callingNode->end();
                                 RI != RE; ++RI) {
        CallGraphNode* callee = RI->second;
        if (Function* FCallee = callee->getFunction()) {
            if (FCallee != M.getFunction("main")) {
                calledFromExternalCode.insert (FCallee);
            }
        }
    }
}


// Print out information for debugging purposes
void PropagateAnalysis::print(raw_ostream &O, const Module *M) const
{
    O << "Propagate Analysis\n";
    for (auto I = FuncCAPTable.begin(), E = FuncCAPTable.end();
         I != E; ++I) {
        Function *F = (*I).first;
        CAPArray_t A = (*I).second;

        if (F) {
            O << F->getName() << ": ";
            dumpCAPArray(O, A);
        }
    }
}

// print out information about the call graph
void PropagateAnalysis::printCG(CallGraph &CG) const {
    std::set<Function *> funcSet;

    for (CallGraph::iterator cgi = CG.begin(); cgi != CG.end(); cgi++) {
        CallGraphNode *node = cgi->second;
        Function *f = node->getFunction();

        funcSet.insert(f);
        
        if (f == NULL) errs() << "NULL's callees: ";
        else errs() << f->getName() << "'s callees: ";
        printCallees(*node);
    }

    /* prints out all functions in the CG */
    errs() << "All functions in the CG: \n";
    for (Function *f : funcSet) {
        if (f != NULL) errs() << f->getName() << "\n";
        else errs() << "NULL\n";
    }

}

// print all callees of a CallGraphNode
void PropagateAnalysis::printCallees(CallGraphNode &CGN) const {
    // print out all functions' callees
    for (CallGraphNode::iterator cgni = CGN.begin(); cgni != CGN.end(); cgni++) {
        CallGraphNode *calleeNode = cgni->second;
        Function *callee = calleeNode->getFunction();
        if (callee != NULL) {
            errs() << callee->getName() << " ";
        }
    }
    errs() << "\n";

}

// register pass
char PropagateAnalysis::ID = 0;
static RegisterPass<PropagateAnalysis> X("PropagateAnalysis", "Privilege Propagate Analysis.",
                                         true, /* CFG only? */
                                         true /* Analysis Pass? */);
