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


/* void DynPrivDstr::getAnalysisUsage(AnalysisUsage &AU) const { */
/*     AU.addRequired<simplifycfg>(); */
/* } */
bool DynPrivDstr::runOnModule(Module &M) {
    errs() << "JZ: Hello from DynPrivDstr Pass!\n";

    // get all basic blocks that has at least one priv_remove call
    std::set<BasicBlock *> targetBBs;
    getFuncUserBB(M.getFunction(PRIV_REMOVE_FUNC), targetBBs);

    // insert the initDynCount function at the beginning of the main function
    insertInitDynCountFunc(M);

    // iterate over all basic blocks 
    for (Module::iterator mi = M.begin(); mi != M.end(); mi++) {
        for (Function::iterator fi = mi->begin(); fi != mi->end(); fi++) {
            BasicBlock *BB = &*fi;
            if (targetBBs.find(BB) != targetBBs.end()) {
                // this basic block has at least one priv_remove call
            } else {
                // this basic block doesn't have any call to priv_remove
                // insert a countBB at the end of this BB
                insertAddBBLOIFunc(M, *BB);
            }
        }
    }

    m = &M;

    return true;  // a transformation pass
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

        unsigned int numArgs =  CI->getNumArgOperands();
        // Note: Skip the first param of priv_lower for it's num of args
        for (unsigned int i = 1; i < numArgs; ++i) {
            // retrieve integer value
            Value *v = CI->getArgOperand(i);
            ConstantInt *I = dyn_cast<ConstantInt>(v);
            /* unsigned int iarg = I->getZExtValue(); */
            uint64_t iarg = I->getZExtValue();

            // Add it to the array
            initialPriv |= ((uint64_t)1u << iarg);
        }
    }
    
    return initialPriv;
}

/*
 * This function finds all the CallInst for a certain function, and stores the basic blocks
 * that contain the CallInst in a set.
 * */
void DynPrivDstr::getFuncUserBB(Function *f, std::set<BasicBlock *> &bbs) {
    for (Value::user_iterator fui = f->user_begin(); fui != f->user_end(); fui++) {
        CallInst *caller = dyn_cast<CallInst>(*fui);
        if (caller == NULL || f != caller->getCalledFunction()) {
            continue;  // why could this happen???
        } 

        // store this priv_remove call's basic block
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

    //
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
void DynPrivDstr::insertAddPrivRmLOIFunc(Module &M, uint64_t LOI, uint64_t removedPriv) {
}


/*
 * This function constructs and inserts the call of addPrivRmLOI function at the end of 
 * a basic block which doesn't contain priv_remove().
 *
 * Please see the code of dynPrivDstr lib for more details of the addPrivRmLOI function.
 * */
void DynPrivDstr::insertAddBBLOIFunc(Module &M, BasicBlock &BB) {
    // step1: construct the prototype if the addBBLOI function
    std::vector<Type *> params;
    IntegerType *int32Type = IntegerType::get(M.getContext(), 32); // parameter: LOI
    Type *voidType = Type::getVoidTy(M.getContext());  // return type: void
    params.push_back(int32Type);

    // declare the function prototype
    FunctionType *addBBLOIFuncType = FunctionType::get(voidType, ArrayRef<Type *>(params), false);
    // create the function prototype
    Function *addBBLOIFunc = dyn_cast<Function>(M.getOrInsertFunction(ADD_BB_LOI_FUNC, addBBLOIFuncType));
    // abort if the construction failed
    assert(addBBLOIFunc && "Constructing addBBLOI function failed!\n");


    // step 2: insert the function call before the last instruction of this basic block
    std::vector<Value *> args;
    ConstantInt *BBLOI = ConstantInt::get(int32Type, BB.size());  // BBLOI: BB's size
    args.push_back(BBLOI);

    // insert the call
    CallInst::Create(addBBLOIFunc, ArrayRef<Value *>(args), "", BB.getTerminator());
}


// construct the prototype of addLOI function
void DynPrivDstr::insertReportPrivDstrFunc(Module &M) {
}


// for the purpose of debugging
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

