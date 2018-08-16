/*
 * This library defines functions that executed instructions dynamically.
 * It is a helper library for the PrivAnalysis passes.
 *
 *
 * @author Jie Zhou
 *
 * */


#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include "dynPrivDstr.h"
#include "priv.h"

#include <map>
#include <set>
#include <iostream>
#include <atomic>

#define CAP_TOTALNUM 37

#define NO_PERMIT 13
#define FILE_NOT_EXIST 2

using namespace std;

static const char *CAPString[CAP_NUM] = {
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

/* #define OUTPUT result_file */
#define OUTPUT stderr
#define MAXBUF 1024

#define DEBUG 0

static pid_t _pid = 0;

// a process's uids and gids 
struct ID {
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;

    ID() {
        getresuid(&ruid, &euid, &suid);
        getresgid(&rgid, &egid, &sgid);
    }

    void update() {
        getresuid(&ruid, &euid, &suid);
        getresgid(&rgid, &egid, &sgid);
    }

    bool equal(struct ID &id) {
        return (id.ruid == ruid && id.euid == euid && id.suid == suid &&
                id.rgid == rgid && id.egid == egid && id.sgid == sgid);
    }

    void dump() const {
        fprintf(stderr, "%d %d %d %d %d %d ", ruid, euid, suid, rgid, egid, sgid);
    }
};

uint64_t total_instr;  // total executed IR
bool removedPrivs[CAP_TOTALNUM + 1];   // removed privilege set during the execution
uint64_t currentPrivSet;
struct ID *currentID;
pair<uint64_t, struct ID> *currentPrivsetID;
std::map<uint64_t, uint64_t> privSetLOIMap;  // map a pair of <privset, ids>
std::map<pair<uint64_t, struct ID> *, uint64_t> privset_id_LOI;

// a helper function for debugging
void calledAtStart() {
    fprintf(stderr, "===JZ: calledAtStart() is called\n");
}

void initDynCount(uint64_t initialPrivSet) {
    char buf[MAXBUF];
    memset(buf, 0, MAXBUF);

#if DEBUG
    /* fprintf(stderr, "\nJZ: in initDynCount(), addBBLOICallCount = %lu,  _pid = %d, getpid() = %d\n", */ 
    /*         addBBLOICallCount, _pid, getpid()); */
#endif

    _pid = getpid();

    total_instr = 0;
    currentPrivSet = initialPrivSet;
    privSetLOIMap.clear();

    privSetLOIMap[currentPrivSet] = 0;

    // initiate removedPrivs
    for (int i = 0; i < CAP_TOTALNUM + 1; i++) {
        if (currentPrivSet & ((uint64_t)1 << i)) removedPrivs[i] = false;
        else removedPrivs[i] = true;
    }

    privset_id_LOI.clear();
    struct ID *newID = new ID();
    currentID = newID;
    currentPrivsetID = new pair<uint64_t, struct ID>(currentPrivSet, *newID);
    privset_id_LOI[currentPrivsetID] = 0;

}

/*
 * addBBLOI adds the number of instructions in a basic block to total instruction counter;
 * it also adds the LOI to the counter for the current privilege set.
 */
void addBBLOI(uint32_t LOI) {
    // first check if this comes from a newly forked child process

    if (_pid != getpid()) {
        // a new process

#if DEBUG
        fprintf(stderr, "===JZ: (from addBBLOI) pid changed! _pid = %d, getpid() = %d\n", _pid, getpid());
#endif

        initDynCount(currentPrivSet);
    }

    total_instr += LOI;
    privSetLOIMap[currentPrivSet] += LOI;

    privset_id_LOI[currentPrivsetID] += LOI;

}

/*
 * addLOI is called when the execution meets a priv_remove call. 
 * It adds the number of instructions executed before the current priv_remove call
 */
void addPrivRmLOI(uint32_t LOI, uint64_t rmPrivSet) {
    if (_pid != getpid()) {

#if DEBUG
        fprintf(stderr, "===JZ: (from addPrivRmLOI) pid changed! _pid = %d, getpid() = %d\n", _pid, getpid());
#endif

        initDynCount(currentPrivSet);
    }

    total_instr += LOI;
    privSetLOIMap[currentPrivSet] += LOI;   

    privset_id_LOI[currentPrivsetID] += LOI;

    uint64_t newPrivSet = getNewPrivSet(rmPrivSet);

    if (newPrivSet != currentPrivSet) {
        currentPrivSet = newPrivSet;
        privSetLOIMap[currentPrivSet] = 0;

        currentPrivsetID = new pair<uint64_t, ID>(currentPrivSet, *currentID);
        privset_id_LOI[currentPrivsetID] = 0;
    }
}

/*
 * A helper function for addPrivRmLOI.
 *
 * It computes the difference between the current privilege set and the newly
 * removed privilege set. The privileges in the newly removed privilege set might 
 * have been removed already.
 * */
uint64_t getNewPrivSet(uint64_t rmPrivSet) {
    /* fprintf("JZ: getNewPrivSet is called\n"); */
    for (int i = 0; i < CAP_TOTALNUM + 1; i++) {
        uint64_t priv = rmPrivSet & ((uint64_t)1 << i);
        if (priv != 0) {
            // remove the ith capability
            if (removedPrivs[i] == true) {
                // this capability has already been removed
                rmPrivSet = rmPrivSet - priv;
            } else {
                // remove it
                removedPrivs[i] = true;
            }
        }
    }

    return currentPrivSet - rmPrivSet;
}


/*
 * When any of setuid, setgid, seteuid,setegid, setresuid, setresgid is called,
 * this function will be called.
 * */
void addIDChangeLOI(uint32_t LOI) {
    total_instr += LOI;
    privset_id_LOI[currentPrivsetID] += LOI;

    ID *newID = new ID();  // "ID newID;" is wrong!
    if (currentID->equal(*newID)) {
        return; // nothing changes
    }
    
    currentID = newID;
    currentPrivsetID = new pair<uint64_t, ID>(currentPrivSet, *currentID);
    privset_id_LOI[currentPrivsetID] = 0;
}


void reportPrivDstr() {
    // report statistics
    fprintf(OUTPUT, "\n========Summary======= pid = %u\n", getpid());
    fprintf(OUTPUT, "Total executed IR number: %lu\n", total_instr);

    /* for (auto privLOI : privSetLOIMap) { */
    /*     dumpPrivSet(privLOI.first); */
    /*     fprintf(OUTPUT, ": %lu, ", privLOI.second); */
    /*     fprintf(OUTPUT, "%.3f%%\n", ((double)privLOI.second / total_instr) * 100); */
    /* } */

    for (auto privDstr : privset_id_LOI) {
        dumpPrivSet(privDstr.first->first);
        fprintf(stderr, "; ");
        privDstr.first->second.dump();
        fprintf(stderr, ": %lu, ", privDstr.second);
        fprintf(stderr, "%.3f%%\n", ((double)(privDstr.second) / total_instr) * 100);
    }

    fprintf(stderr, "\n");
}

// dump CAPArray for debugging purpose
void dumpPrivSet(uint64_t privSet) {
    char buf[MAXBUF];
    memset(buf, 0, MAXBUF);

    if (privSet == 0) {
        sprintf(buf, "empty");
        do_log(buf);
        /* fprintf(OUTPUT, "empty"); */
        /* fflush(OUTPUT); */
        return;
    }

    for (int i = 0; i < CAP_TOTALNUM + 1; ++i) {
        if (privSet& ((uint64_t)1 << i)) {
            sprintf(buf + strlen(buf), "%s ", CAPString[i]);
            /* fprintf(OUTPUT, "%s ", CAPString[i]); */
        }
    }
    do_log(buf);
}


void do_log(const char *str) {
    const char *file_name = "/home/jzhou41/result_file.log";

    /* fprintf(result_file, "%s", str); */
    fprintf(stderr, "%s", str);
    /* fclose(result_file); */
}



void forkHandler() {
#if DEBUG
    fprintf(stderr, "===JZ: fork() is called by process %d\n", getpid());
#endif
}
