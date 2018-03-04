// ====-------------- FindSigHandlers.h ---------*- C++ -*----====
//
// Identify functions that are signal handlers
//
// ====-------------------------------------------------------====

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <set>

using namespace llvm;

struct FindSignalHandlers : public ModulePass {
  public:
    static char ID;

    // Determine whether function is signal handler
    bool isSignalHandler (const Function * F) {
      return (SignalHandlers.find (F) != SignalHandlers.end());
    }

    // Constructor
    FindSignalHandlers();

    // Run on Module Start
    virtual bool runOnModule(Module &M);

    // Preserve analysis usage
    void getAnalysisUsage(AnalysisUsage &AU) const;

    // Print out information for debugging purposes
    void print(raw_ostream &O, const Module *M) const;

  private:
    // List of functions identified as signal handlers
    std::set <const Function *> SignalHandlers;

};
