// ====----------  PrivRemoveInsert.cpp ----------*- C++ -*---====
//
// Based on the information from GlobalLiveAnalysis, insert 
// privRemove calls at the end of BasicBlocks, to remove 
// unnecessary privileges. 
//
// ====-------------------------------------------------------====


#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>

#include <map>

#include "ADT.h"
#include "PrivRemoveInsert.h"
#include "GlobalLiveAnalysis.h"
#include "FindSignalHandlers.h"


using namespace llvm;
using namespace llvm::globalLiveAnalysis;
using namespace llvm::privremoveinsert;


// PrivRemoveInsert constructor
PrivRemoveInsert::PrivRemoveInsert() : ModulePass(ID)
{ }


// Preserve analysis usage
void PrivRemoveInsert::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<GlobalLiveAnalysis>();
    AU.addRequired<FindSignalHandlers>();
}


// Initialization
// param M: the Module class
bool PrivRemoveInsert::doInitialization(Module &M)
{
    return false;
}


// Get PrivRemove function 
// param M: the Module class
Function *PrivRemoveInsert::getRemoveFunc(Module &M)
{
    std::vector<Type *> Params;
    Type *IntType = IntegerType::get(getGlobalContext(), 32);
    Params.push_back(IntType);
    FunctionType *RemoveCallType = FunctionType::get(IntType,
                                                     ArrayRef<Type *>(Params),
                                                     true);
    Constant *PrivRemoveCall = M.getOrInsertFunction(PRIV_REMOVE_CALL,
                                                     RemoveCallType);
    
    return dyn_cast<Function>(PrivRemoveCall);
}


// Insert params to function type
// param: Args - the Args vector to insert into
//        CAPArray - the array of CAP to 
void PrivRemoveInsert::addToArgs(std::vector<Value *>& Args,
                                 const CAPArray_t &CAPArray)
{
    int cap_num = 0;
    int cap = 0;

    for (cap = 0; cap < CAP_TOTALNUM; ++cap) {
        if ((CAPArray & (1 << cap)) == 0) {
            continue;
        }

        // add to args vector
        Constant *arg = ConstantInt::get
            (IntegerType::get(getGlobalContext(), 32), cap);
        Args.push_back(arg);

        cap_num++;
    }

    // Add the number of args to the front
    ConstantInt *arg_num = ConstantInt::get
        (IntegerType::get(getGlobalContext(), 32), cap_num);
    Args.insert(Args.begin(), arg_num);

    return;
}

static inline bool
instrumentBasicBlock (const BasicBlock * BB) {
	if (isa<UnreachableInst>(BB->getTerminator())) {
		return false;
	}
	if (const CallInst * CI = dyn_cast<CallInst>(BB->getFirstNonPHI())) {
		if (const Function * F = CI->getCalledFunction()) {
			if (F->getName().str() == "llvm.trap") {
				return false;
			}
		}
	}

	return true;
}

// Run on Module
bool PrivRemoveInsert::runOnModule(Module &M)
{
    GlobalLiveAnalysis &GA = getAnalysis<GlobalLiveAnalysis>();
    BBCAPTable_t BBCAPTable_dropEnd = GA.BBCAPTable_dropEnd;
    BBCAPTable_t BBCAPTable_dropStart = GA.BBCAPTable_dropStart;

    FuncCAPTable_t FuncLiveCAPTable_in = GA.FuncLiveCAPTable_in;

    // Insert remove call at the top of the main function
    Function *PrivRemoveFunc = getRemoveFunc(M);
    std::vector<Value *> Args = {};

    //
    // Find all of the signal handlers.
    //
    FindSignalHandlers &SigHandlers = getAnalysis<FindSignalHandlers>();

    // Insert call to all BBs with removable capabilities  
    for (auto BI = BBCAPTable_dropEnd.begin(), BE = BBCAPTable_dropEnd.end();
         BI != BE; ++BI) {
        BasicBlock *BB = BI->first;
        CAPArray_t &CAPArray = BI->second;
        Args.clear();

        addToArgs(Args, CAPArray);

	//
	// If the basic block belongs to a signal handler, do not instrument it.
	//
	if (SigHandlers.isSignalHandler (BB->getParent())) {
		continue;
	}

        // create call instruction
        assert(BB->getTerminator() != NULL && "BB has a NULL teminator!");
	if (instrumentBasicBlock (BB)) {
		CallInst::Create(PrivRemoveFunc, ArrayRef<Value *>(Args), 
				 "", BB->getTerminator());
	}
    }


    // Insert at the start of the dropStart
    for (auto BI = BBCAPTable_dropStart.begin(), BE = BBCAPTable_dropStart.end();
         BI != BE; ++BI) {
        BasicBlock *BB = BI->first;
        CAPArray_t &CAPArray = BI->second;
        Args.clear();

        addToArgs(Args, CAPArray);

	//
	// If the basic block belongs to a signal handler, do not instrument it.
	//
	if (SigHandlers.isSignalHandler (BB->getParent())) {
		continue;
	}

        // create call instruction
	if (instrumentBasicBlock (BB)) {
		CallInst::Create(PrivRemoveFunc, ArrayRef<Value *>(Args), 
				 "", BB->getFirstNonPHI());
	}
    }

    return true;
}


// Print out information for debugging purposes
void PrivRemoveInsert::print(raw_ostream &O, const Module *M) const
{
}



// register pass
char PrivRemoveInsert::ID = 0;
static RegisterPass<PrivRemoveInsert> I("PrivRemoveInsert", "Insert PrivRemove calls", 
                                        true, /* CFG only? */
                                        false /* Analysis pass? */);

