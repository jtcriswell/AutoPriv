// ====------------ FindSigHandlers.cpp ---------*- C++ -*----====
//
// Identify functions that are signal handlers
//
// ====-------------------------------------------------------====

#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

#include "FindSignalHandlers.h"


using namespace llvm;

// constructor
FindSignalHandlers::FindSignalHandlers() : ModulePass(ID) { }

void
FindSignalHandlers::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

//
// Method: runOnModule()
//
// Description:
//  Given a reference to a Module, find all functions that are registered
//  as signal handlers.
//
bool
FindSignalHandlers::runOnModule (Module &M) {
  //
  // Find the signal() function.  If it does not exist, then there are no signal
  // handlers.
  //
  Function * Signal = M.getFunction ("signal");

  //
  // Iterate through all uses of the signal function and find the functions
  // that are registered as signal handlers.
  //
  for (Value::user_iterator UI = Signal->user_begin(), UE = Signal->user_end();
       UI != UE; ++UI) {
    if (CallInst *CI = dyn_cast<CallInst>(*UI)) {
      if (CI && ((CI->getCalledFunction()->stripPointerCasts()) == Signal)) {
        CallSite CS(CI);
        Value * SigHandler = CS.getArgument(1)->stripPointerCasts();
        if (Function * F = dyn_cast<Function>(SigHandler)) {
          SignalHandlers.insert(F);
        }
      }
    }
  }

  return false;
}


//
// Method: print()
//
// Description:
//   Print the list of identified signal handlers to the specified output
//   stream.
//
void
FindSignalHandlers::print(raw_ostream &O, const Module *M) const {
  std::set <const Function *>::iterator fi = SignalHandlers.begin();
  std::set <const Function *>::iterator fe = SignalHandlers.end();
  for (; fi != fe; ++fi) {
    O << (*fi)->getName().str() << "\n";
  }
}

// Register the pass
char FindSignalHandlers::ID = 0;
static RegisterPass<FindSignalHandlers> D("findsighandlers",
                                          "Find Signal Handlers", 
                                          true,
                                          true);
