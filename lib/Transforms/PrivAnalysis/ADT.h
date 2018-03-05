// ====-----------------------  ADT.h -----------*- C++ -*---====
//
// The ADT and configuration for the PrivAnalysis
//
// ====-------------------------------------------------------====

#ifndef __ADT_H__
#define __ADT_H__

#include "llvm/IR/Module.h"

#include <linux/capability.h>
#include <cstdint>

#include <map>
#include <array>
#include <unordered_map>
#include <string>

// Constant Definition
#define TARGET_FUNC  "priv_raise"
#define PRIVRAISE    "priv_raise"
#define PRIVLOWER    "priv_lower"
#define CAP_TOTALNUM (CAP_LAST_CAP + 1)

namespace llvm {
namespace privAnalysis {


// type definition

// using 64bit long unsigned to represent all 37 capabilities
// typedef std::array<bool, CAP_TOTALNUM> CAPArray_t;
typedef uint64_t CAPArray_t;

// The map from functions to CAPArray
typedef std::unordered_map<Function*, CAPArray_t> FuncCAPTable_t;

// The map from functions to CAPArray
typedef std::unordered_map<Function*, CAPArray_t> FuncCAPTable_t;

// The map from basicblocks to CAPArray
typedef std::unordered_map<BasicBlock*, CAPArray_t> BBCAPTable_t;

// The map from basicblocks to functions
typedef std::unordered_map<BasicBlock*, Function*> BBFuncTable_t;

// The unique capabiltiy set for all basic blocks mapped to the number of its CAPs
typedef std::map<CAPArray_t, int> CAPSet_t;

// --------------------------- //
// Data manipulation functions
// --------------------------- //

//
// Add to the FuncCAPTable, if Func exists, merge the CAPArray
// param: CAPTable - ref to the FuncCAPTable
//        F - the function to add
//        CAParray - the array of capability to add to FuncCAPTable
static inline void
AddToFuncCAPTable(FuncCAPTable_t &CAPTable, Function *F, CAPArray_t CAParray) {
    // If not found in map, add to map
    // else, Union the two arrays
    if (CAPTable.find(F) == CAPTable.end()) {
        CAPTable[F] = CAParray;
    } else {
        CAPTable[F] |= CAParray;
    }
}

// Add to the BBCAPTable, if BB exists, merge the CAPArray
// param: CAPTable - ref to the BBCAPTable
//        B - the BasicBlock to add
//        CAParray - the array of capability to add to FuncCAPTable
static inline void
AddToBBCAPTable(BBCAPTable_t &CAPTable, BasicBlock *B, CAPArray_t CAParray) {
    // If not found in map, add to map
    // else, Union the two arrays
    if (CAPTable.find(B) == CAPTable.end() ) {
        CAPTable[B] = CAParray;
    } else {
        CAPTable[B] |= CAParray;
    }
}

// Copy CAPTable keys from src to dest
// After this operation, dest would have all keys src
// has, with empty CAPArrays mapping to each keys
// param: dest - dest CAPTable
//        src  - src CAPTable
static inline void
CopyTableKeys(FuncCAPTable_t &dest, const FuncCAPTable_t &src) {
    CAPArray_t emptyArray = 0;

    for(auto MI = src.begin(), ME = src.end(); MI != ME; ++ MI) {
  
        Function *tf = MI->first;

        dest[tf] = emptyArray;
    }
}

// ------------------- //
// Array manipulations
// ------------------- //

// Find the size of the input array
// param: A - the input array
// return the number of the capablities inside CAPArray 
static inline int
findCAPArraySize(const CAPArray_t &A) {
    // to be refactored
    int size = 0;
    for (int i = 0; i < CAP_TOTALNUM; ++i){
        size += ((uint64_t)1 << i) & A;
    }

    return size;
}

// Union two arrays, save result to dest
// param: dest - dest CAPArray
//        src  - src CAPArray
// return: ischanged - if the dest's value is changed
static inline bool
UnionCAPArrays(CAPArray_t &dest, const CAPArray_t &src) {
    bool ischanged = false;

    ischanged = (~dest & src);
    dest = (dest | src);

    return ischanged;
}

// Diff two arrays, save result to dest
// param: dest - dest CAPArray
//        A - the CAPArray to subtract from
//        B - the CAPArray to subtract
// return: if there is difference between A and B
static inline bool
DiffCAPArrays(CAPArray_t &dest, const CAPArray_t &A, const CAPArray_t &B) {
    assert( (A | ~B) && "\nBUG in Diff CAPArrays!\n\n");
    dest = A & ~B;
    return (bool)dest;
} 


// Find the reverse of cap array
// param: the CAParray to reverse
static inline void
ReverseCAPArray(CAPArray_t &A) {
    A = ~A;
}

// If the CAPArray is empty
// @A: the CAPArray to examine
// return: if it's empty
static inline bool
IsCAPArrayEmpty(const CAPArray_t &A) {
    return (A == 0);
}

// Dump CAPArray 
void dumpCAPArray(raw_ostream &O, const CAPArray_t &A);

void dumpCAPArray(const CAPArray_t &A);

// dump CAPTable for Debugging purpose
// param: CT - the CAPTable to dump
void dumpCAPTable(const FuncCAPTable_t &CT);

} // namespace privAnalysis
} // namepsace llvm

#endif
