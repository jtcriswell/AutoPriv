// ====-----------------------  ADT.h -----------*- C++ -*---====
//
// The ADT and configuration for the PrivAnalysis
//
// ====------------------------------------------------------====

#include "ADT.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <array>
#include <algorithm>
#include <utility>


namespace llvm {
namespace privAnalysis {


// capability number to string for ROSA
const char *CAPString[CAP_TOTALNUM] = {
    "CapChown",
    "CapDacOverride",
    "CapDacReadSearch",
    "CapFowner",
    "CapFsetid",
    "CapKill",
    "CapSetgid",
    "CapSetuid",
    "CapSetpCap",
    "CapLinuxImmutable",
    "CapNetBindService",
    "CapNetBroadcast",
    "CapNetAdmin",
    "CapNetRaw",
    "CapIpcLock",
    "CapIpcOwner",
    "CapSysModule",
    "CapSysRawio",
    "CapSysChroot",
    "CapSysPtrace",
    "CapSysPacct",
    "CapSysAdmin",
    "CapSysBoot",
    "CapSysNice",
    "CapSysResource",
    "CapSysTime",
    "CapSysTtyConfig",
    "CapMknod",
    "CapLease",
    "CapAuditWrite",
    "CapAuditControl",
    "CapSetfCap",
    "CapMacOverride",
    "CapMacAdmin",
    "CapSyslog",
    "CapWakeAlarm",
    "CapBlockSuspend"
};

  
// dump CAPArray for debugging purpose
void dumpCAPArray(raw_ostream &O, const CAPArray_t &A) {
    if (A == 0) {
        O << "empty,";
    }

    for (int i = 0; i < CAP_TOTALNUM; ++i) {
        if (A & ((uint64_t)1 << i)) {
            O << CAPString[i] << ",";
        }
    }

    O << "\n";
}

void dumpCAPArray(const CAPArray_t &A) {
    
    dumpCAPArray(errs(), A);

}


// dump CAPTable for Debugging purpose
// param: CT - the CAPTable to dump
void dumpCAPTable(const FuncCAPTable_t &CT)
{
    // iterate through captable, a map from func to array
    for (auto mi = CT.begin(), me = CT.end();
         mi != me; ++ mi){

        errs() << mi->first->getName() << " Privileges:\t";

        // iterate through cap array
        for (int i = 0; i < CAP_TOTALNUM; ++i) {
            if (mi->second & ((uint64_t)1 << i)) {
                errs() << CAPString[i] << "\t";
            }
        }
        errs() << "\n";
    }
    errs() << "\n";
}


} // namespace privAnalysis
} // namepsace llvm
