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

#include "DynPrivDistribution.h"

using namespace llvm;
using namespace llvm::privAnalysis;
using namespace llvm::dynPrivDstr;



// constructor
DynPrivDstr::DynPrivDstr() : ModulePass(ID) {
}


// runOnFunction
bool DynPrivDstr::runOnModule(Module &M) {
    // insert addBBLOI or addPrivRmLOI at proper places
    insertAddLOIFunc(M);
    
    // insert the initDynCount function at the beginning of the main function
    insertInitDynCountFunc(M);

    // insert the reportPrivDistr function
    insertAtexitFunc(M);

    /* insertInitFunc(M); */
    
    return true;  // a transformation pass
}

/*
 * This function insert addBBLOI or addPrivRmLOI at proper locations.
 * There are three special functions calls: priv_remove, execve, and fork
 * that need extra care. 
 * If a basic block doesn't have any call to the special functions, 
 * then just insert an addBBLOI at the end (before the last IR) of the BB;
 * if it calls priv_remove, then insert a addPrivRmLOI() before each such call;
 * if it calls execve family functions, then insert addBBLOI and reportPrivDistr before each such call;
 * if it calls fork, then insert addBBLOI before each such call.
 *
 * Some BB has an UnreachableInst as the last instruction; in these cases,
 * insert the count instrunction before the real executed last instruction; 
 * otherwise the count function won't get executed.
 * */
void DynPrivDstr::insertAddLOIFunc(Module &M) {
    std::set<BasicBlock *> privRmBBs;  // BBs that have priv_remove
    std::set<BasicBlock *> forkBBs;    // BBs that have fork
    std::set<BasicBlock *> execBBs;  // BBs that have execve
    getFuncUserBB(M.getFunction(PRIV_REMOVE_FUNC), privRmBBs);
    getFuncUserBB(M.getFunction(FORK_FUNC), forkBBs);

    // get all basic blocks that call a exec family function
    std::set<std::string> execFuncs{"execve", "execl", "execlp", "execle", "execv", "execvp", "execvpe" };
    for (std::string execFunc : execFuncs) {
        getFuncUserBB(M.getFunction(execFunc), execBBs);
    }

    // iterate over all basic blocks 
    for (Module::iterator mi = M.begin(); mi != M.end(); mi++) {
        for (Function::iterator fi = mi->begin(); fi != mi->end(); fi++) {
            BasicBlock *BB = &*fi;
            uint32_t LOI = 0;   // line of instruction
            if (privRmBBs.find(BB) != privRmBBs.end() || 
                    execBBs.find(BB) != execBBs.end() ||
                    forkBBs.find(BB) != forkBBs.end()) {
                // this basic block has at least one special call
                for (BasicBlock::iterator bbi = BB->begin(); bbi != BB->end(); bbi++) {
                    // JZ: I don't like variables names like I, CI, func, 
                    // but sometimes it's really hard to find good names!
                    CallInst *CI = dyn_cast<CallInst>(&*bbi);
                    if (CI != NULL) {
                        Function *func = CI->getCalledFunction();
                        if (func != NULL) {
                            if (func->getName().equals(PRIV_REMOVE_FUNC)) {
                                insertAddPrivRmLOIFunc(M, CI, LOI + 1, getPrivSetFromPrivPrimitives(CI));
                                LOI = -1;  // regard the call to priv_remove in the previous priv set
                            } else if (execFuncs.find(func->getName()) != execFuncs.end()) {
                                // insert an addBBLOI and a reportPrivDistr
                                insertAddBBLOIFunc(M, CI, LOI + 1);
                                CallInst::Create(getReportPrivDstrFunc(M), "", CI);
                                LOI = -1;
                            } else if (func->getName().equals(FORK_FUNC)) {
                                // insert an addBBLOI
                                insertAddBBLOIFunc(M, CI, LOI + 1);
                                insertForkHandler(M, CI);
                                LOI = -1;
                            }
                        }
                    } 
                    LOI++;
                }
            } else {
                // this basic block doesn't have any special calls
                LOI = BB->size();
            }
            Instruction *lastInst = BB->getTerminator();
            if (isa<UnreachableInst>(lastInst)) {
                // The last instruction of this BB is an UnreachableInst.
                // We need insert addBBLOI before the real last executed instruction.
                Instruction *realLastInst = dyn_cast<Instruction>(lastInst->getPrevNode());
                insertAddBBLOIFunc(M, realLastInst, LOI - 1);
            } else {
                insertAddBBLOIFunc(M, lastInst, LOI);
            }
        }
    }
}

// get privilege set from a privilege primitive function call
uint64_t DynPrivDstr::getPrivSetFromPrivPrimitives(CallInst *CI) const {
    uint64_t privSet = 0;
    for (unsigned int i = 1; i < CI->getNumArgOperands(); i++) {
        uint64_t priv = (dyn_cast<ConstantInt>(CI->getArgOperand(i)))->getZExtValue();
        privSet |= ((uint64_t)1 << priv);
    }

    return privSet;
}

/*
 * getInitialPrivSet() finds all the available privileges in the permissible set 
 * at the beginning of a program.
 *
 * */
uint64_t DynPrivDstr::getInitialPrivSet(Module &M) {
    uint64_t initialPriv = 0;
    Function *priv_raise_func = M.getFunction(PRIVRAISE);
    for (Value::user_iterator prui = priv_raise_func->user_begin(); prui != priv_raise_func->user_end(); prui++) {
        CallInst *CI = dyn_cast<CallInst>(*prui);
        assert(CI != NULL && "The CallInst is NULL!\n");

        initialPriv |= getPrivSetFromPrivPrimitives(CI);
    }
    
    return initialPriv;
}

/*
 * This function finds all the CallInst for a certain function, and stores the basic blocks
 * that contain the CallInst in a set.
 * */
void DynPrivDstr::getFuncUserBB(Function *f, std::set<BasicBlock *> &bbs) {
    if (f == NULL) return;

    for (Value::user_iterator fui = f->user_begin(); fui != f->user_end(); fui++) {
        CallInst *caller = dyn_cast<CallInst>(*fui);
        if (caller == NULL || f != caller->getCalledFunction()) {
            continue;  // why could this happen???
        } 

        // store this function call's basic block
        bbs.insert(caller->getParent());
    }
}


/*
 * This function constructs and inserts a call of the initDynCount function at the beginning 
 * of the main function.
 *
 * Please see the code of dynPrivDstr lib for more details of the initDynCount function.
 * */
void DynPrivDstr::insertInitDynCountFunc(Module &M) {
    // step 1: construct the prototype of the initDynCount function
    std::vector<Type *> params;  // parameters
    IntegerType *int64Type = IntegerType::get(M.getContext(), 64);  // parameter: initial privilege set
    Type *voidType = Type::getVoidTy(M.getContext());  // return type: void
    /* FunctionType *voidFuncType = FunctionType::get(voidType, */ 
    params.push_back(int64Type);

    // declare the function prototype
    FunctionType *initDynCountFuncType = FunctionType::get(voidType, ArrayRef<Type *>(params), false);
    // create the function prototype
    Function *initDynCountFunc = dyn_cast<Function>(M.getOrInsertFunction(INIT_COUNT_FUNC, initDynCountFuncType));
    // abort if the construction failed
    assert(initDynCountFunc && "Constructing initDynCount function failed!\n");

    // step 2: insert this function call with the initialPriv at the begining of main function
    std::vector<Value *> args;  // arguments
    uint64_t initialPriv = getInitialPrivSet(M);  // get initial privilege set
    /* dumpCAPArray(initialPriv); */
    ConstantInt *initPrivArgs = ConstantInt::get(int64Type, initialPriv);  // initial privilege set
    args.push_back(initPrivArgs);

    // insert this call
    Function *mainFunc = M.getFunction(MAIN_FUNC);
    CallInst::Create(initDynCountFunc, ArrayRef<Value *>(args), "", mainFunc->getEntryBlock().getFirstNonPHI());
    
}


// construct the prototype of initDynCount function
void DynPrivDstr::insertAddPrivRmLOIFunc(Module &M, Instruction *I, uint32_t LOI, uint64_t removedPriv) {
    // step1: construct the prototype for the addPrivRmLOI function
    std::vector<Type *> params;
    IntegerType *int32Type = IntegerType::get(M.getContext(), 32);  // LOI
    IntegerType *int64Type = IntegerType::get(M.getContext(), 64);  // removedPrivSet
    Type *voidType = Type::getVoidTy(M.getContext());  // return type: void
    params.push_back(int32Type);
    params.push_back(int64Type);

    // declare the function prototype
    FunctionType *addPrivRmLOIFuncType = FunctionType::get(voidType, ArrayRef<Type *>(params), false);
    // create the function type
    Function *addPrivRmLOIFunc = dyn_cast<Function>(M.getOrInsertFunction(ADD_PRIV_RM_LOI_FUNC, addPrivRmLOIFuncType));
    // abort if the construction failed
    assert(addPrivRmLOIFunc && "Constructing addPrivRmLOI function failed!\n");

    // step 2: insert the function call before the last instruction of this basic block
    std::vector<Value *> args;
    ConstantInt *LOIValue = ConstantInt::get(int32Type, LOI);
    ConstantInt *removedPrivValue = ConstantInt::get(int64Type, removedPriv);
    args.push_back(LOIValue);
    args.push_back(removedPrivValue);

    // insert the call
    CallInst::Create(addPrivRmLOIFunc, ArrayRef<Value *>(args), "", I);
}


/*
 * This function constructs and inserts the call of addPrivRmLOI function at the end of 
 * a basic block which doesn't contain priv_remove().
 *
 * Please see the code of dynPrivDstr lib for more details of the addPrivRmLOI function.
 * */
void DynPrivDstr::insertAddBBLOIFunc(Module &M, Instruction *insertBefore, uint32_t LOI) {
    // step1: construct the prototype for the addBBLOI function
    std::vector<Type *> params;
    IntegerType *int32Type = IntegerType::get(M.getContext(), 32); // parameter: LOI
    Type *voidType = Type::getVoidTy(M.getContext());  // return type: void
    params.push_back(int32Type);

    /* ArrayType *arrayType = ArrayType::get(IntegerType::get(M.getContext(), 8), funcName.size()); */
    /* params.push_back(arrayType); */

    // declare the function prototype
    FunctionType *addBBLOIFuncType = FunctionType::get(voidType, ArrayRef<Type *>(params), false);
    // create the function prototype
    Function *addBBLOIFunc = dyn_cast<Function>(M.getOrInsertFunction(ADD_BB_LOI_FUNC, addBBLOIFuncType));
    // abort if the construction failed
    assert(addBBLOIFunc && "Constructing addBBLOI function failed!\n");

    // step 2: insert the function call before the last instruction of this basic block
    std::vector<Value *> args;
    ConstantInt *BBLOI = ConstantInt::get(int32Type, LOI);  // BBLOI: BB's size
    args.push_back(BBLOI);

    // insert the call
    CallInst::Create(addBBLOIFunc, ArrayRef<Value *>(args), "", insertBefore);
}


/*
 * This function constructs and inserts the reportPrivDistr function before the program exits 
 * and before it calls execve family functions.
 *
 * A corner case is that a basic block has both priv_remove and execve. 
 * */
void DynPrivDstr::insertAtexitFunc(Module &M) {
    // create reportPrivDstr prototype
    Type *voidType = Type::getVoidTy(M.getContext());
    FunctionType *reportPrivDstrFuncType = FunctionType::get(voidType, false);

    // construct the atexit function prototype
    std::vector<Type *> params;  // for parameter of atexit
    params.push_back(reportPrivDstrFuncType->getPointerTo());
    IntegerType *int32Type = IntegerType::get(M.getContext(), 32); // return type of atexit: int
    FunctionType *atexitFuncType = FunctionType::get(int32Type, ArrayRef<Type *>(params), false);
    Function *atexitFunc = dyn_cast<Function>(M.getOrInsertFunction(ATEXIT_FUNC, atexitFuncType));
    assert(atexitFunc && "Constructing atexitFunc function failed!\n");

    std::vector<Value *> args;  //args for atexit
    args.push_back(getReportPrivDstrFunc(M));
    
    // insert the call to atexit
    Function *mainFunc = M.getFunction(MAIN_FUNC);
    CallInst::Create(atexitFunc, ArrayRef<Value *>(args), "", mainFunc->getEntryBlock().getFirstNonPHI());
}

// a helper method for insertReportPrivDstrFunc and insertAtexitFunc
Function *DynPrivDstr::getReportPrivDstrFunc(Module &M) {
    // construct the prototype of reportPrivDistr function
    Type *voidType = Type::getVoidTy(M.getContext());
    // declare the function prototype
    FunctionType *reportPrivDstrFuncType = FunctionType::get(voidType, false);
    // create the function prototype
    Constant *targetFunc = M.getOrInsertFunction(REPORT_PRIV_DSTR_FUNC, reportPrivDstrFuncType);
    Function *reportPrivDstrFunc = dyn_cast<Function>(targetFunc);
    // abort if the construction failed
    assert(reportPrivDstrFunc && "Constructing reportPrivDistr function failed!\n");
    
    return reportPrivDstrFunc;
}

// construct and insert call to forkHandler function
void DynPrivDstr::insertForkHandler(Module &M, Instruction *insertBefore) {
    Type *voidType = Type::getVoidTy(M.getContext());
    FunctionType *forkHandlerFuncType = FunctionType::get(voidType, false);
    Constant *targetFunc = M.getOrInsertFunction("forkHandler", forkHandlerFuncType);
    Function *forkHandlerFunc = dyn_cast<Function>(targetFunc);
    CallInst::Create(forkHandlerFunc, "", insertBefore);
}

// construct and insert call to calledAtStart function
void DynPrivDstr::insertInitFunc(Module &M) {
    Type *voidType = Type::getVoidTy(M.getContext());
    FunctionType *calledAtStartFuncType = FunctionType::get(voidType, false);
    Constant *targetFunc = M.getOrInsertFunction("calledAtStart", calledAtStartFuncType);
    Function *calledAtStartFunc = dyn_cast<Function>(targetFunc);
    Function *mainFunc = M.getFunction(MAIN_FUNC);
    CallInst::Create(calledAtStartFunc, "", mainFunc->getEntryBlock().getFirstNonPHI());
}

// for the purpose of debugging
void DynPrivDstr::print(raw_ostream &O, Module &M) const {
    for (Module::iterator mi = M.begin(); mi != M.end(); mi++) {
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

